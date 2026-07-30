#pragma once
#include "Command.hpp"
#include <cstdint>
#include <future>
#include <memory>

struct Job {
    uint64_t id;
    Command cmd;
    std::shared_ptr<std::promise<int>> result_promise;
};
