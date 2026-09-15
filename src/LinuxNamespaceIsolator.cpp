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
#include <seccomp.h>
#include <sys/mman.h>
#include <atomic>

static std::atomic<int> file_counter{1};

struct CloneArgs {
    const char *executable;
    char **argv;
    const char *rootfs_path;
    const char *input_file;
    const char *output_file;
    int sync_fd;
    scmp_filter_ctx seccomp_ctx;
    char host_out_file[64];
    char host_err_file[64];
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
    } else if (args->host_out_file[0] != '\0') {
        int fd = open(args->host_out_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd != -1) {
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }
    }

    if (args->host_err_file[0] != '\0') {
        int fd = open(args->host_err_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd != -1) {
            dup2(fd, STDERR_FILENO);
            close(fd);
        }
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

    setpgid(0, 0);

    if (args->seccomp_ctx != nullptr) {
        if (seccomp_load(args->seccomp_ctx) < 0) {
            perror("[Isolyx Child] seccomp_load failed");
            return -1;
        }
    }

    execvp(args->executable, args->argv);

    perror("[Isolyx Child] execvp failed");
    return -1;
}

LinuxNamespaceIsolator::LinuxNamespaceIsolator(std::unique_ptr<IRootfsProvider> rootfs_provider)
    : rootfs_provider_(std::move(rootfs_provider)) {}

ExecutionResult LinuxNamespaceIsolator::isolateAndRun(
    const Command &cmd, pid_t& pid, int& unique_id, int time_quantum, 
    IResourceLimiter* limiter, IWatchdog* watchdog, ISecurityPolicy* sec_policy) {
    ExecutionResult result;
    if (cmd.isEmpty())
        return result;

    std::string absolute_rootfs = rootfs_provider_->prepareRootfs();

    if (pid <= 0) {
        std::vector<char *> raw_args;
        for (const auto &arg : cmd.arguments) {
            raw_args.push_back(const_cast<char *>(arg.c_str()));
        }
        raw_args.push_back(nullptr);

        int sync_pipe[2];
        if (pipe(sync_pipe) == -1) {
            perror("[Isolyx] pipe() failed");
            return result;
        }

        CloneArgs c_args;
        c_args.executable = cmd.executable.c_str();
        c_args.argv = raw_args.data();
        c_args.rootfs_path = absolute_rootfs.c_str();
        c_args.output_file = cmd.redirectOutput.empty() ? nullptr : cmd.redirectOutput.c_str();
        c_args.input_file = cmd.redirectInput.empty() ? nullptr : cmd.redirectInput.c_str();
        c_args.sync_fd = sync_pipe[0];
        c_args.seccomp_ctx = sec_policy ? sec_policy->getContext() : nullptr;
        
        if (unique_id == 0) {
            unique_id = file_counter.fetch_add(1);
        }
        snprintf(c_args.host_out_file, sizeof(c_args.host_out_file), "/tmp/isolyx_out_%d", unique_id);
        snprintf(c_args.host_err_file, sizeof(c_args.host_err_file), "/tmp/isolyx_err_%d", unique_id);

        auto stack = std::make_unique<char[]>(STACK_SIZE);
        char *stack_top = stack.get() + STACK_SIZE;

        int flags = CLONE_NEWPID | CLONE_NEWUTS | CLONE_NEWNS | CLONE_NEWUSER | CLONE_NEWIPC | SIGCHLD;

        pid_t child_pid = clone(child_entry, stack_top, flags, &c_args);

        if (child_pid == -1) {
            perror("[Isolyx] clone() failed");
            return result;
        }
        pid = child_pid;

        if (limiter) {
            limiter->applyToPid(child_pid);
        }

        if (watchdog) {
            watchdog->start(child_pid);
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
    } else {
        // Process already exists, just resume it
        kill(-pid, SIGCONT);
    }

    int status;
    bool finished = false;

    if (time_quantum > 0) {
        int elapsed_ms = 0;
        while (true) {
            pid_t w = waitpid(pid, &status, WNOHANG | WUNTRACED);
            if (w == pid) {
                if (WIFEXITED(status)) {
                    result.exit_code = WEXITSTATUS(status);
                    result.success = (result.exit_code == 0);
                    finished = true;
                    break;
                } else if (WIFSIGNALED(status)) {
                    result.term_signal = WTERMSIG(status);
                    result.success = false;
                    finished = true;
                    break;
                } else if (WIFSTOPPED(status)) {
                    result.preempted = true;
                    finished = true;
                    break;
                }
            } else if (w == -1) {
                perror("[Isolyx] waitpid() failed");
                return result;
            }

            if (elapsed_ms >= time_quantum) {
                kill(-pid, SIGSTOP); // Send STOP to process group
            } else {
                usleep(1000);
                elapsed_ms++;
            }
        }
    } else {
        if (waitpid(pid, &status, 0) == -1) {
            perror("[Isolyx] waitpid() failed");
            return result;
        }
        finished = true;
        if (WIFEXITED(status)) {
            result.exit_code = WEXITSTATUS(status);
            result.success = (result.exit_code == 0);
        } else if (WIFSIGNALED(status)) {
            result.term_signal = WTERMSIG(status);
            result.success = false;
        }
    }

    if (finished && !result.preempted) {
        if (watchdog) {
            watchdog->stop();
        }
        rootfs_provider_->teardownRootfs();

        auto read_and_delete = [](const std::string &path) -> std::string {
            int fd = open(path.c_str(), O_RDONLY);
            if (fd == -1) return "";
            std::string out;
            char buf[4096];
            ssize_t bytes;
            while ((bytes = read(fd, buf, sizeof(buf))) > 0) {
                out.append(buf, bytes);
            }
            close(fd);
            unlink(path.c_str());
            return out;
        };

        if (unique_id > 0) {
            std::string out_path = absolute_rootfs + "/tmp/isolyx_out_" + std::to_string(unique_id);
            std::string err_path = absolute_rootfs + "/tmp/isolyx_err_" + std::to_string(unique_id);
            std::cerr << "[Isolyx] Trying to read: " << out_path << std::endl;
            result.stdout_out = read_and_delete(out_path);
            result.stderr_out = read_and_delete(err_path);
        }
    }

    return result;
}