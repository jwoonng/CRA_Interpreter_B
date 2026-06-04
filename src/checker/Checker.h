#pragma once
#include <stdexcept>
#include <string>
#include <unordered_map>
#include "IChecker.h"   // -> src/common/Stmt.h -> src/common/Expr.h -> Token.h

// CheckError: Checker가 발생시키는 의미 분석 오류
struct CheckError : std::runtime_error {
    int line;
    CheckError(int line, const std::string& msg)
        : std::runtime_error("[" + std::to_string(line) + "번째 줄] " + msg), line(line) {}
};

// Checker는 IChecker를 구현하고, StmtVisitor/ExprVisitor를 통해 AST를 순회한다.
// StmtVisitor/ExprVisitor는 구현 세부사항이므로 private 상속
class Checker : public IChecker, private StmtVisitor, private ExprVisitor {
public:
    void check(const std::vector<std::unique_ptr<Stmt>>& stmts) override;

private:
    // false = 선언됨(미초기화), true = 초기화 완료
    std::vector<std::unordered_map<std::string, bool>> scopes_;

    void beginScope();
    void endScope();
    void declare(const Token& name);
    void define(const Token& name);
    void resolveVar(const Token& name);

    // ── StmtVisitor ──────────────────────────────────────────────
    void visitExpressionStmt(ExpressionStmt& s) override;
    void visitPrintStmt(PrintStmt& s)           override;
    void visitVarDeclareStmt(VarDeclareStmt& s) override;
    void visitBlockStmt(BlockStmt& s)           override;
    void visitIfStmt(IfStmt& s)                 override;
    void visitForStmt(ForStmt& s)               override;

    // ── ExprVisitor (반환값은 사용하지 않음) ────────────────────
    LiteralValue visitLiteralExpr(LiteralExpr& e)     override;
    LiteralValue visitVariableExpr(VariableExpr& e)   override;
    LiteralValue visitAssignExpr(AssignExpr& e)       override;
    LiteralValue visitBinaryExpr(BinaryExpr& e)       override;
    LiteralValue visitUnaryExpr(UnaryExpr& e)         override;
    LiteralValue visitGroupingExpr(GroupingExpr& e)   override;
    LiteralValue visitLogicalExpr(LogicalExpr& e)     override;
};
