#pragma once
#include <string>
#include <variant>

enum class TokenType {
    LEFT_PAREN, RIGHT_PAREN, LEFT_BRACE, RIGHT_BRACE, SEMICOLON,
    PLUS, MINUS, STAR, SLASH,
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

using LiteralValue = std::variant<std::monostate, double, std::string, bool>;

struct Token {
    TokenType    type;
    std::string  lexeme;
    LiteralValue literal;
    int          line;
};
