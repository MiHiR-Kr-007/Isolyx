#include <iostream>
#include <fstream>
#include <string>

int main() {
    std::ifstream passwd("/etc/passwd");
    if (passwd.is_open()) {
        std::string line;
        while (std::getline(passwd, line)) {
            std::cout << line << std::endl;
        }
        passwd.close();
        return 0;
    } else {
        std::cerr << "Failed to open /etc/passwd" << std::endl;
        return 1;
    }
}
