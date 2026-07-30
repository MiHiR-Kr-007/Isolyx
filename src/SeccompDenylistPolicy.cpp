#include "SeccompDenylistPolicy.hpp"
#include <iostream>

SeccompDenylistPolicy::SeccompDenylistPolicy() : ctx_(nullptr) { init(); }

SeccompDenylistPolicy::~SeccompDenylistPolicy() {
    if (ctx_) {
        seccomp_release(ctx_);
    }
}

void SeccompDenylistPolicy::init() {
    ctx_ = seccomp_init(SCMP_ACT_ALLOW);
    if (!ctx_) {
        std::cerr << "[Seccomp] Failed to initialize seccomp context\n";
        return;
    }

    // kill immediately if attempted
    seccomp_rule_add(ctx_, SCMP_ACT_KILL, SCMP_SYS(ptrace), 0);
    seccomp_rule_add(ctx_, SCMP_ACT_KILL, SCMP_SYS(reboot), 0);
    seccomp_rule_add(ctx_, SCMP_ACT_KILL, SCMP_SYS(mount), 0);
    seccomp_rule_add(ctx_, SCMP_ACT_KILL, SCMP_SYS(umount2), 0);
    seccomp_rule_add(ctx_, SCMP_ACT_KILL, SCMP_SYS(unshare), 0);
    seccomp_rule_add(ctx_, SCMP_ACT_KILL, SCMP_SYS(socket), 0);
}

scmp_filter_ctx SeccompDenylistPolicy::getContext() { return ctx_; }
