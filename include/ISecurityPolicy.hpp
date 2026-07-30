#pragma once

typedef void* scmp_filter_ctx;

class ISecurityPolicy {
public:
    virtual ~ISecurityPolicy() = default;
    
    virtual void init() = 0;
    virtual scmp_filter_ctx getContext() = 0;
};
