#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "src/shell/Shell.h"
#include <string>

int main(int argc, char** argv) {
#ifdef NDEBUG
    // Release 빌드: 항상 Shell 실행
    Shell shell;
    shell.run(std::cin, std::cout);
    return 0;
#else
    // Debug 빌드: --shell 플래그 시 Shell, 그 외 gtest
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--shell") {
            Shell shell;
            shell.run(std::cin, std::cout);
            return 0;
        }
    }
    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
#endif
}
