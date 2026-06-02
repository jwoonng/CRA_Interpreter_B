#pragma once
#include <gmock/gmock.h>
#include "src/assembler/IParser.h"

struct MockParser : IParser {
    // MSVC: 템플릿 반환 타입은 using 별칭으로 처리해야 구문 오류 없음
    using ParseResult = std::vector<std::unique_ptr<Stmt>>;
    MOCK_METHOD(ParseResult, parse, (const std::vector<Token>& tokens), (override));
};
