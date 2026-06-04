#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "src/shell/Shell.h"

// Debug 빌드 전용 — Release 빌드는 main.cpp 사용
int main(int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--shell") {
            Shell shell;
            shell.run(std::cin, std::cout);
            return 0;
        }
    }
    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}
