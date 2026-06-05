#pragma once
#include <memory>
#include <string>
#include <variant>
#include <vector>

enum class TokenType {
    LEFT_PAREN, RIGHT_PAREN, LEFT_BRACE, RIGHT_BRACE, SEMICOLON,
    PLUS, MINUS, STAR, SLASH, PERCENT,
    BANG, EQUAL, EQUAL_EQUAL, BANG_EQUAL,
    GREATER, GREATER_EQUAL, LESS, LESS_EQUAL,
    IDENTIFIER, STRING, NUMBER,
    VAR, IF, ELSE, FOR, TRUE_KW, FALSE_KW, AND, OR, PRINT,

    // ── 함수 관련 (예정) ──────────────────────────────────────
    FUN, RETURN, COMMA,

    // ── 배열 관련 (예정) ──────────────────────────────────────
    LEFT_BRACKET, RIGHT_BRACKET,

    EOF_TOKEN
};

// ── 배열 값 타입 (LiteralValue 내 자기참조를 shared_ptr로 처리) ───
struct LiteralArray;
using ArrayPtr = std::shared_ptr<LiteralArray>;

using LiteralValue = std::variant<std::monostate, double, std::string, bool, ArrayPtr>;

// LiteralValue 완전 정의 이후에 구체화
struct LiteralArray {
    std::vector<LiteralValue> elements;
    explicit LiteralArray(std::size_t n) : elements(n) {}
};

struct Token {
    TokenType    type;
    std::string  lexeme;
    LiteralValue literal;
    int          line;
};
