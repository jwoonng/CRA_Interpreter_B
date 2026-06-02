#pragma once
#include <string>
#include <vector>
#include "src/common/Token.h"

// Assembler Unit — Tokenizer 인터페이스
// 담당: 개발자 1
struct ITokenizer {
    virtual ~ITokenizer() = default;

    // 소스코드 → Token 목록
    virtual std::vector<Token> tokenize(const std::string& source) = 0;
};
