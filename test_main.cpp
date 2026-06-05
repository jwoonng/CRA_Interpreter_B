#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "src/shell/ShellLauncher.h"
#include <iostream>
#include <string>

// Debug 빌드 전용 — Release 빌드는 main.cpp 사용
// 기본은 Google Test 실행, '--shell' 이후 인자는 공장 제어 쉘로 전달한다.
//   Project17.exe --shell                    REPL 모드
//   Project17.exe --shell <script>           파일 모드
//   Project17.exe --shell --debug <script>   디버그 모드
int main(int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--shell") {
            // argv[i] 를 프로그램 이름 자리로 사용해 이후 인자를 그대로 전달
            return launchShell(argc - i, argv + i, std::cin, std::cout);
        }
    }
    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}
