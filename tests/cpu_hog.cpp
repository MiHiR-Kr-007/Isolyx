#include <iostream>
#include <unistd.h>
#include <chrono>

int main() {
    auto start = std::chrono::steady_clock::now();
    while (true) {
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - start).count() > 10) {
            break; // self terminate after 10s
        }
    }
    return 0;
}
