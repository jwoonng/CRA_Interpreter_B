#pragma once
#include <gmock/gmock.h>
#include "src/assembler/ITokenizer.h"

struct MockTokenizer : ITokenizer {
    MOCK_METHOD(std::vector<Token>, tokenize, (const std::string& source), (override));
};
