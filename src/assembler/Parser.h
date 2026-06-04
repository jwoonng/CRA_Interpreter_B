#pragma once
#include "IParser.h"
#include <functional>
#include <initializer_list>
#include <stdexcept>

class Parser : public IParser {
public:
    std::vector<std::unique_ptr<Stmt>> parse(const std::vector<Token>& tokens) override;

private:
    const std::vector<Token>* tokens_  = nullptr;
    int                       current_ = 0;

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

    // ── 이진 표현식 공통 헬퍼 ────────────────
    ExprPtr parseBinaryLeft(std::function<ExprPtr()> next,
                            std::initializer_list<TokenType> ops);
    ExprPtr parseLogicalLeft(std::function<ExprPtr()> next, TokenType opType);

    // ── 유틸리티 ─────────────────────────────
    const Token&       peek()     const;
    const Token&       previous() const;
    bool               isAtEnd()  const;
    void               advance();
    bool               check(TokenType type);
    bool               match(std::initializer_list<TokenType> types);
    Token              consume(TokenType type, const std::string& message);
    std::runtime_error error(const Token& token, const std::string& message);
};
