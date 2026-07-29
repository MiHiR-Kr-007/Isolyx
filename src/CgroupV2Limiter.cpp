#include "CgroupV2Limiter.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <signal.h>
#include <system_error>
#include <unistd.h>

namespace fs = std::filesystem;

CgroupV2Limiter::CgroupV2Limiter(long long max_cpu_us, long long cpu_period_us, long long max_memory_bytes,
                                 long long max_pids) {
    std::string base_path = getBaseCgroupPath();

    std::string subtree_path = base_path + "/cgroup.subtree_control";
    std::ofstream subtree_out(subtree_path, std::ios::app);
    if (subtree_out.is_open()) {
        subtree_out << "+cpu +memory +pids\n";
        subtree_out.close();
    }

    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<uint32_t> dist;

    cgroup_path_ = base_path + "/isolyx_" + std::to_string(dist(gen));

    std::error_code ec;
    fs::create_directories(cgroup_path_, ec);
    if (ec) {
        std::cerr << "[Isolyx/Cgroups] Failed to create cgroup directory: " << cgroup_path_ << "\n";
    }

    if (max_cpu_us > 0) {
        std::ofstream cpu_max(cgroup_path_ + "/cpu.max");
        if (cpu_max.is_open()) {
            cpu_max << max_cpu_us << " " << cpu_period_us << "\n";
        } else {
            std::cerr << "[Isolyx/Cgroups] Failed to configure cpu.max\n";
        }
    }

    if (max_memory_bytes > 0) {
        std::ofstream mem_max(cgroup_path_ + "/memory.max");
        if (mem_max.is_open()) {
            mem_max << max_memory_bytes << "\n";
        } else {
            std::cerr << "[Isolyx/Cgroups] Failed to configure memory.max\n";
        }

        std::ofstream swap_max(cgroup_path_ + "/memory.swap.max");
        if (swap_max.is_open()) {
            swap_max << "0\n";
        }
    }

    if (max_pids > 0) {
        std::ofstream pids_max(cgroup_path_ + "/pids.max");
        if (pids_max.is_open()) {
            pids_max << max_pids << "\n";
        } else {
            std::cerr << "[Isolyx/Cgroups] Failed to configure pids.max\n";
        }
    }
}

CgroupV2Limiter::~CgroupV2Limiter() {
    std::ifstream procs(cgroup_path_ + "/cgroup.procs");
    pid_t pid;
    while (procs >> pid) {
        kill(pid, SIGKILL);
    }
    procs.close();

    std::error_code ec;
    for (int i = 0; i < 10; ++i) {
        if (fs::remove(cgroup_path_, ec)) {
            break;
        }
        usleep(20000);
    }
    if (ec) {
        std::cerr << "[Isolyx/Cgroups] Failed to clean up cgroup directory: " << ec.message() << "\n";
    }
}

void CgroupV2Limiter::applyToPid(pid_t pid) {
    std::ofstream procs(cgroup_path_ + "/cgroup.procs");
    if (procs.is_open()) {
        procs << pid << "\n";
    } else {
        std::cerr << "[Isolyx/Cgroups] Failed to open cgroup.procs to apply pid\n";
    }
}

std::string CgroupV2Limiter::getBaseCgroupPath() {
    std::ifstream f("/proc/self/cgroup");
    std::string line;
    if (std::getline(f, line)) {
        auto pos = line.find_last_of(':');
        if (pos != std::string::npos) {
            std::string path = line.substr(pos + 1);
            fs::path p = "/sys/fs/cgroup";
            p += path;
            p = p.parent_path();
            return p.string();
        }
    }
    return "/sys/fs/cgroup/isolyx";
}
