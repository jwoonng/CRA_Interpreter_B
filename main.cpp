#include "src/shell/ShellLauncher.h"
#include <iostream>

// Release 빌드 전용 진입점 — 공장 제어 쉘 실행
//   (인자 없음)        REPL 모드
//   <script>           파일 모드
//   --debug <script>   디버그 모드
int main(int argc, char* argv[]) {
    return launchShell(argc, argv, std::cin, std::cout);
}
