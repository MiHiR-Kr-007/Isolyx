#include <iostream>
#include <vector>
#include <unistd.h>

int main() {
    std::cout << "Starting memory bomb\n";
    std::vector<char *> ptrs;
    while (true) {
        char *p = new char[1024 * 1024]; // 1MB
        for (int i = 0; i < 1024 * 1024; ++i) {
            p[i] = 1;
        }
        ptrs.push_back(p);
        usleep(10000);
    }
    return 0;
}
