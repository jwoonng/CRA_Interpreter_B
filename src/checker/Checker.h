#pragma once
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "IChecker.h"
#include "src/common/Expr.h"
#include "src/common/Stmt.h"

// CheckError: Checker가 발생시키는 의미 분석 오류
struct CheckError : std::runtime_error {
    int line;
    CheckError(int line, const std::string& msg)
        : std::runtime_error("[" + std::to_string(line) + "번째 줄] " + msg), line(line) {}
};

class Checker : public IChecker, public ExprVisitor, public StmtVisitor {
public:
    void check(const std::vector<std::unique_ptr<Stmt>>& stmts) override;

private:
    // false = declared(uninitialized), true = initialized
    std::vector<std::unordered_map<std::string, bool>> scopes_;
    int functionDepth_ = 0;

    void beginScope();
    void endScope();
    void declare(const Token& name);
    void define(const Token& name);
    void resolveVar(const Token& name);
    void checkStmt(Stmt& s);
    void checkExpr(Expr& e);

    // StmtVisitor 구현
    void visitExpressionStmt(ExpressionStmt& s) override;
    void visitPrintStmt(PrintStmt& s) override;
    void visitVarDeclareStmt(VarDeclareStmt& s) override;
    void visitBlockStmt(BlockStmt& s) override;
    void visitIfStmt(IfStmt& s) override;
    void visitForStmt(ForStmt& s) override;
    void visitFunctionStmt(FunctionStmt& s) override;
    void visitReturnStmt(ReturnStmt& s) override;

    // ExprVisitor 구현 (의미 검사용 — 반환값은 dummy)
    LiteralValue visitLiteralExpr(LiteralExpr& e) override;
    LiteralValue visitVariableExpr(VariableExpr& e) override;
    LiteralValue visitAssignExpr(AssignExpr& e) override;
    LiteralValue visitBinaryExpr(BinaryExpr& e) override;
    LiteralValue visitUnaryExpr(UnaryExpr& e) override;
    LiteralValue visitGroupingExpr(GroupingExpr& e) override;
    LiteralValue visitLogicalExpr(LogicalExpr& e) override;
    LiteralValue visitCallExpr(CallExpr& e) override;
};
