#include "src/shell/Shell.h"
#include <iostream>

// Release 빌드 전용 진입점 — Shell을 직접 실행
int main() {
    Shell shell;
    shell.run(std::cin, std::cout);
    return 0;
}
