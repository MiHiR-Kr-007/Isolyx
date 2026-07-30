#pragma once
#include "IIsolator.hpp"
#include "IRootfsProvider.hpp"
#include <memory>

class LinuxNamespaceIsolator : public IIsolator {
public:
    explicit LinuxNamespaceIsolator(std::unique_ptr<IRootfsProvider> rootfs_provider);
    ~LinuxNamespaceIsolator() override = default;

    int isolateAndRun(const Command &cmd, IResourceLimiter* limiter = nullptr, IWatchdog* watchdog = nullptr, ISecurityPolicy* sec_policy = nullptr) override;

private:
    std::unique_ptr<IRootfsProvider> rootfs_provider_;

    // 1 MB stack for the cloned child process
    static constexpr size_t STACK_SIZE = 1024 * 1024;
};