#include "LinuxNamespaceIsolator.hpp"
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <iostream>
#include <sched.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

struct CloneArgs {
    const char *executable;
    char **argv;
    const char *rootfs_path;
    const char *input_file;
    const char *output_file;
    int sync_fd;
};

#include <stdio.h>

static int child_entry(void *arg) {
    CloneArgs *args = static_cast<CloneArgs *>(arg);

    char sync_byte;
    if (read(args->sync_fd, &sync_byte, 1) != 1) {
        perror("[Isolyx Child] Failed to read sync byte from parent");
        return -1;
    }
    close(args->sync_fd);

    const char *custom_hostname = "isolyx-guest";
    if (sethostname(custom_hostname, strlen(custom_hostname)) != 0) {
        perror("[Isolyx Child] sethostname failed");
        return -1;
    }

    if (mount(NULL, "/", NULL, MS_REC | MS_PRIVATE, NULL) == -1) {
        perror("[Isolyx Child] mount / private failed");
        return -1;
    }

    if (mount(args->rootfs_path, args->rootfs_path, "bind", MS_BIND | MS_REC, NULL) == -1) {
        perror("[Isolyx Child] bind mount failed (Does rootfs_base exist here?)");
        return -1;
    }

    if (chdir(args->rootfs_path) == -1) {
        perror("[Isolyx Child] chdir to rootfs failed");
        return -1;
    }

    char proc_path[512];
    snprintf(proc_path, sizeof(proc_path), "%s/proc", args->rootfs_path);
    if (mount("proc", proc_path, "proc", 0, NULL) == -1) {
        perror("[Isolyx Child] mount /proc before pivot_root failed");
    }

    const char *old_root_dir = "oldroot";
    mkdir(old_root_dir, 0777);

    if (syscall(SYS_pivot_root, ".", old_root_dir) == -1) {
        perror("[Isolyx Child] pivot_root failed");
        return -1;
    }

    if (chdir("/") == -1) {
        perror("[Isolyx Child] chdir to / failed");
        return -1;
    }

    if (umount2(old_root_dir, MNT_DETACH) == -1) {
        perror("[Isolyx Child] umount2 oldroot failed");
        return -1;
    }

    rmdir(old_root_dir);

    if (args->output_file != nullptr) {
        int fd = open(args->output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd == -1) {
            perror("[Isolyx Child] Failed to open output file");
            return -1;
        }
        dup2(fd, STDOUT_FILENO);
        close(fd);
    }

    if (args->input_file != nullptr) {
        int fd = open(args->input_file, O_RDONLY);
        if (fd == -1) {
            perror("[Isolyx Child] Failed to open input file");
            return -1;
        }
        dup2(fd, STDIN_FILENO);
        close(fd);
    }

    setenv("PATH", "/bin:/usr/bin:/sbin:/usr/sbin", 1);

    execvp(args->executable, args->argv);

    perror("[Isolyx Child] execvp failed");
    return -1;
}

LinuxNamespaceIsolator::LinuxNamespaceIsolator(std::unique_ptr<IRootfsProvider> rootfs_provider)
    : rootfs_provider_(std::move(rootfs_provider)) {}

int LinuxNamespaceIsolator::isolateAndRun(const Command &cmd) {
    if (cmd.isEmpty())
        return -1;

    std::vector<char *> raw_args;
    for (const auto &arg : cmd.arguments) {
        raw_args.push_back(const_cast<char *>(arg.c_str()));
    }
    raw_args.push_back(nullptr);

    std::string absolute_rootfs = rootfs_provider_->prepareRootfs();

    int sync_pipe[2];
    if (pipe(sync_pipe) == -1) {
        perror("[Isolyx] pipe() failed");
        return -1;
    }

    CloneArgs c_args;
    c_args.executable = cmd.executable.c_str();
    c_args.argv = raw_args.data();
    c_args.rootfs_path = absolute_rootfs.c_str();
    c_args.output_file = cmd.redirectOutput.empty() ? nullptr : cmd.redirectOutput.c_str();
    c_args.input_file = cmd.redirectInput.empty() ? nullptr : cmd.redirectInput.c_str();
    c_args.sync_fd = sync_pipe[0];

    auto stack = std::make_unique<char[]>(STACK_SIZE);
    char *stack_top = stack.get() + STACK_SIZE;

    int flags = CLONE_NEWPID | CLONE_NEWUTS | CLONE_NEWNS | CLONE_NEWUSER | CLONE_NEWIPC | SIGCHLD;

    pid_t child_pid = clone(child_entry, stack_top, flags, &c_args);

    if (child_pid == -1) {
        perror("[Isolyx] clone() failed");
        return -1;
    }

    close(sync_pipe[0]);

    uid_t host_uid = getuid();
    gid_t host_gid = getgid();
    if (const char *sudo_uid = getenv("SUDO_UID")) {
        host_uid = std::stoi(sudo_uid);
    }
    if (const char *sudo_gid = getenv("SUDO_GID")) {
        host_gid = std::stoi(sudo_gid);
    }

    char map_buf[100];
    snprintf(map_buf, sizeof(map_buf), "0 %d 1\n", host_uid);

    std::string uid_map_path = "/proc/" + std::to_string(child_pid) + "/uid_map";
    int fd = open(uid_map_path.c_str(), O_WRONLY);
    if (fd != -1) {
        write(fd, map_buf, strlen(map_buf));
        close(fd);
    } else {
        perror("[Isolyx] failed to write uid_map");
    }

    std::string setgroups_path = "/proc/" + std::to_string(child_pid) + "/setgroups";
    fd = open(setgroups_path.c_str(), O_WRONLY);
    if (fd != -1) {
        write(fd, "deny", 4);
        close(fd);
    }

    snprintf(map_buf, sizeof(map_buf), "0 %d 1\n", host_gid);
    std::string gid_map_path = "/proc/" + std::to_string(child_pid) + "/gid_map";
    fd = open(gid_map_path.c_str(), O_WRONLY);
    if (fd != -1) {
        write(fd, map_buf, strlen(map_buf));
        close(fd);
    } else {
        perror("[Isolyx] failed to write gid_map");
    }

    if (write(sync_pipe[1], "1", 1) != 1) {
        perror("[Isolyx] failed to write to sync pipe");
    }
    close(sync_pipe[1]);

    int status;
    if (waitpid(child_pid, &status, 0) == -1) {
        perror("[Isolyx] waitpid() failed");
        return -1;
    }

    rootfs_provider_->teardownRootfs();

    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }
    return -1;
}