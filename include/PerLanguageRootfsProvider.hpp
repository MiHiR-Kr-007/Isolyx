#pragma once
#include "IRootfsProvider.hpp"

class PerLanguageRootfsProvider : public IRootfsProvider {
public:
    PerLanguageRootfsProvider() = default;
    ~PerLanguageRootfsProvider() override = default;

    std::string prepareRootfs() override;
    void teardownRootfs() override;
};
