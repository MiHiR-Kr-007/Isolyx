#include "PerLanguageRootfsProvider.hpp"
#include <filesystem>
#include <iostream>

std::string PerLanguageRootfsProvider::prepareRootfs() {
    std::string absolute_rootfs = std::filesystem::absolute("rootfs_base").string();
    return absolute_rootfs;
}

void PerLanguageRootfsProvider::teardownRootfs() {}
