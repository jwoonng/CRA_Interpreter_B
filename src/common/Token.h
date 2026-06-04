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
    EOF_TOKEN
};

using Value = std::variant<std::monostate, double, std::string, bool>;

struct Token {
    TokenType    type;
    std::string  lexeme;
    Value literal;
    int          line;
};
