#pragma once
#include "IParser.h"
#include <initializer_list>
#include <stdexcept>

class Parser : public IParser {
public:
    std::vector<std::unique_ptr<Stmt>> parse(const std::vector<Token>& tokens) override;

private:
    std::vector<Token> tokens_;
    int current_ = 0;

    // ── 문장 ─────────────────────────────────
    StmtPtr statement();
    StmtPtr forStatement();
    StmtPtr ifStatement();
    StmtPtr printStatement();
    StmtPtr varDeclaration();
    StmtPtr block();
    StmtPtr expressionStatement();

    // ── 표현식 (우선순위 낮은 순) ─────────────
    ExprPtr expression();
    ExprPtr assignment();
    ExprPtr logicalOr();
    ExprPtr logicalAnd();
    ExprPtr equality();
    ExprPtr comparison();
    ExprPtr term();
    ExprPtr factor();
    ExprPtr unary();
    ExprPtr primary();

    // ── 유틸리티 ─────────────────────────────
    const Token&       peek();
    const Token&       previous();
    bool               isAtEnd();
    Token              advance();
    bool               check(TokenType type);
    bool               match(std::initializer_list<TokenType> types);
    Token              consume(TokenType type, const std::string& message);
    std::runtime_error error(const Token& token, const std::string& message);
};
