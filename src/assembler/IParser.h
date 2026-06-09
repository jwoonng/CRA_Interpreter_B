#pragma once
#include <memory>
#include <vector>
#include "src/common/Token.h"
#include "src/common/Stmt.h"

// Assembler Unit — Parser 인터페이스
// 담당: 개발자 2
struct IParser {
    virtual ~IParser() = default;

    // Token 목록 → 문법 Tree (Stmt 목록)
    virtual std::vector<std::unique_ptr<Stmt>> parse(const std::vector<Token>& tokens) = 0;

    // 토큰 목록에서 pos 위치부터 문장 하나를 파싱 후 pos를 갱신한다.
    // EOF에 도달하면 nullptr 반환.
    virtual std::unique_ptr<Stmt> parseOne(const std::vector<Token>& tokens, int& pos) = 0;
};
