#pragma once
#include "src/common/Token.h"
#include "src/common/Expr.h"
#include "src/common/Stmt.h"
#include <memory>
#include <string>
#include <vector>

// Anonymous namespace gives internal linkage per TU — avoids global name
// collisions (e.g., MSVC may expose eof() from Windows/CRT headers).
namespace {

// ── Token factories ───────────────────────────────────────────────────────────
Token tok(TokenType type, std::string lexeme, LiteralValue lit = {}, int line = 1) {
    return Token{ type, std::move(lexeme), std::move(lit), line };
}
Token num(double v, int line = 1) {
    return tok(TokenType::NUMBER, std::to_string(v), v, line);
}
Token str(std::string s, int line = 1) {
    return tok(TokenType::STRING, s, s, line);
}
Token id(std::string s, int line = 1) {
    return tok(TokenType::IDENTIFIER, std::move(s), {}, line);
}
Token eof(int line = 1) {
    return tok(TokenType::EOF_TOKEN, "", {}, line);
}

// ── Expr builders ─────────────────────────────────────────────────────────────
ExprPtr varExpr(const std::string& name, int line = 1) {
    return std::make_unique<VariableExpr>(id(name, line));
}
ExprPtr litNum(double v, int line = 1) {
    return std::make_unique<LiteralExpr>(v, line);
}
ExprPtr litStr(std::string s, int line = 1) {
    return std::make_unique<LiteralExpr>(std::move(s), line);
}
ExprPtr binExpr(ExprPtr l, TokenType op, std::string opLex, ExprPtr r, int line = 1) {
    return std::make_unique<BinaryExpr>(
        std::move(l), tok(op, std::move(opLex), {}, line), std::move(r));
}

// ── Stmt builders ─────────────────────────────────────────────────────────────
StmtPtr printStmt(ExprPtr e, int line = 1) {
    return std::make_unique<PrintStmt>(std::move(e), line);
}
StmtPtr varDecl(const std::string& name, ExprPtr init = nullptr, int line = 1) {
    return std::make_unique<VarDeclareStmt>(id(name, line), std::move(init));
}
StmtPtr exprStmt(ExprPtr e) {
    return std::make_unique<ExpressionStmt>(std::move(e));
}

} // namespace

// ── AST casting helpers (templates — outside anonymous namespace for linkage) ─
template<typename T> T* as(Stmt* s) { return dynamic_cast<T*>(s); }
template<typename T> T* as(Expr* e) { return dynamic_cast<T*>(e); }
