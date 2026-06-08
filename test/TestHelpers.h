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

// ── Array builders ────────────────────────────────────────────────────────────
// array(n) 호출 식 생성 — LiteralValue 오버로드 (문자열 크기 오류 케이스에 사용)
ExprPtr makeArrayCall(LiteralValue sizeLit, int line = 1) {
    std::vector<ExprPtr> args;
    args.push_back(std::make_unique<LiteralExpr>(std::move(sizeLit), line));
    return std::make_unique<CallExpr>(
        varExpr("array", line),
        tok(TokenType::RIGHT_PAREN, ")", {}, line),
        std::move(args));
}
// array(n) 호출 식 생성 — double 오버로드 (일반 사용)
ExprPtr makeArrayCall(double n, int line = 1) {
    return makeArrayCall(LiteralValue{n}, line);
}
// unique_ptr<IndexExpr> 반환 — IndexAssignExpr 생성에 사용 (타입 안전)
std::unique_ptr<IndexExpr> makeIndexExpr(ExprPtr obj, ExprPtr idx, int line = 1) {
    return std::make_unique<IndexExpr>(
        std::move(obj), std::move(idx),
        tok(TokenType::RIGHT_BRACKET, "]", {}, line));
}
// ExprPtr 반환 — 읽기 컨텍스트(printStmt 등)에 사용
ExprPtr makeIndex(ExprPtr obj, ExprPtr idx, int line = 1) {
    return makeIndexExpr(std::move(obj), std::move(idx), line);
}
// arr[idx] = val — unique_ptr<IndexExpr>로 타입 레벨에서 IndexExpr 강제
ExprPtr makeIndexAssign(std::unique_ptr<IndexExpr> target, ExprPtr val, int line = 1) {
    return std::make_unique<IndexAssignExpr>(std::move(target), std::move(val), line);
}

// ── Function builders ─────────────────────────────────────────────────────────
StmtPtr retStmt(ExprPtr val = nullptr, int line = 1) {
    return std::make_unique<ReturnStmt>(
        tok(TokenType::RETURN, "return", {}, line), std::move(val));
}
StmtPtr makeFuncDecl(
    const std::string& name,
    std::vector<std::string> paramNames,
    std::vector<StmtPtr> body,
    int line = 1)
{
    std::vector<Token> params;
    for (auto& p : paramNames)
        params.push_back(id(p, line));
    return std::make_unique<FunctionStmt>(
        id(name, line), std::move(params), std::move(body));
}
ExprPtr makeCallExpr(
    const std::string& funcName,
    std::vector<ExprPtr> args,
    int line = 1)
{
    return std::make_unique<CallExpr>(
        varExpr(funcName, line),
        tok(TokenType::RIGHT_PAREN, ")", {}, line),
        std::move(args));
}
StmtPtr makeCallStmt(
    const std::string& funcName,
    std::vector<ExprPtr> args,
    int line = 1)
{
    return std::make_unique<ExpressionStmt>(
        makeCallExpr(funcName, std::move(args), line));
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
