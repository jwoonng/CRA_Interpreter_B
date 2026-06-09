#pragma once
#include "IParser.h"
#include <initializer_list>
#include <span>
#include <stdexcept>

class Parser : public IParser {
public:
    std::vector<std::unique_ptr<Stmt>> parse(const std::vector<Token>& tokens) override;
    std::unique_ptr<Stmt>              parseOne(const std::vector<Token>& tokens, int& pos) override;

private:
    std::span<const Token> tokens_;
    int                    current_ = 0;

    // ── 문장 ─────────────────────────────────
    StmtPtr statement();
    StmtPtr forStatement();
    StmtPtr ifStatement();
    StmtPtr printStatement();
    StmtPtr varDeclaration();
    StmtPtr block();
    StmtPtr expressionStatement();
    StmtPtr functionDeclaration();
    StmtPtr returnStatement();

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
    ExprPtr call();
    ExprPtr finishCall(ExprPtr callee);
    ExprPtr primary();

    // ── 이진 표현식 공통 헬퍼 ─────────────────
    // NodeType: BinaryExpr 또는 LogicalExpr (생성자 시그니처 동일)
    // Fn      : () → ExprPtr (다음 우선순위 파싱 함수)
    template<typename NodeType, typename Fn>
    ExprPtr parseBinaryLeft(Fn next, std::initializer_list<TokenType> ops) {
        ExprPtr expr = next();
        while (match(ops)) {
            Token   op  = previous();
            ExprPtr rhs = next();
            expr = std::make_unique<NodeType>(std::move(expr), std::move(op), std::move(rhs));
        }
        return expr;
    }

    // ── 유틸리티 ─────────────────────────────
    const Token&       peek()     const;
    const Token&       previous() const;
    bool               isAtEnd()  const;
    void               advance();
    bool               check(TokenType type) const;
    bool               match(std::initializer_list<TokenType> types);
    Token              consume(TokenType type, const std::string& message);
    std::runtime_error error(const Token& token, const std::string& message) const;
};
