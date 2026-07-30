#pragma once
#include "ISecurityPolicy.hpp"
#include <seccomp.h>

class SeccompDenylistPolicy : public ISecurityPolicy {
public:
    SeccompDenylistPolicy();
    ~SeccompDenylistPolicy() override;

    void init() override;
    scmp_filter_ctx getContext() override;

private:
    scmp_filter_ctx ctx_;
};
