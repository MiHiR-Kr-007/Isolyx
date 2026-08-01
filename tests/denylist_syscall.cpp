#include <iostream>
#include <sys/ptrace.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main() {
    std::cout << "Attempting to call ptrace" << std::endl;
    if (ptrace(PTRACE_TRACEME, 0, NULL, NULL) == -1) {
        perror("ptrace failed");
        return 1;
    }
    std::cout << "ptrace succeeded" << std::endl;
    return 0;
}
