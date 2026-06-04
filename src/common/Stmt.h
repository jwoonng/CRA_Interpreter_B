#pragma once
#include "Expr.h"
#include <vector>
#include <memory>
#include <optional>

// 전방 선언
class StmtVisitor;
struct ExpressionStmt;
struct PrintStmt;
struct VarDeclareStmt;
struct BlockStmt;
struct IfStmt;
struct ForStmt;

// ── Stmt 기반 클래스 ─────────────────────────────────────────
struct Stmt {
    int line = 0;
    virtual ~Stmt() = default;
    virtual void accept(StmtVisitor& v) = 0;
};

using StmtPtr = std::unique_ptr<Stmt>;

// ── Stmt 서브 클래스 ─────────────────────────────────────────

// Expr를 Stmt로 감싸는 Wrapper
struct ExpressionStmt : Stmt {
    ExprPtr expression;
    explicit ExpressionStmt(ExprPtr expression)
        : expression(std::move(expression)) {
        if (this->expression) line = this->expression->line;
    }
    void accept(StmtVisitor& v) override;
};

// print expr;
struct PrintStmt : Stmt {
    ExprPtr expression;
    int printLine;
    PrintStmt(ExprPtr expression, int printLine)
        : expression(std::move(expression)), printLine(printLine) {
        line = printLine;
    }
    void accept(StmtVisitor& v) override;
};

// var name = initializer;
struct VarDeclareStmt : Stmt {
    Token name;
    ExprPtr initializer; // nullable
    VarDeclareStmt(Token name, ExprPtr initializer = nullptr)
        : name(std::move(name)), initializer(std::move(initializer)) {
        line = this->name.line;
    }
    void accept(StmtVisitor& v) override;
};

// { stmt* }
struct BlockStmt : Stmt {
    std::vector<StmtPtr> statements;
    explicit BlockStmt(std::vector<StmtPtr> statements)
        : statements(std::move(statements)) {
    }
    void accept(StmtVisitor& v) override;
};

// if (cond) thenBranch [else elseBranch]
struct IfStmt : Stmt {
    ExprPtr condition;
    StmtPtr thenBranch;
    StmtPtr elseBranch; // nullable
    IfStmt(ExprPtr condition, StmtPtr thenBranch, StmtPtr elseBranch = nullptr)
        : condition(std::move(condition))
        , thenBranch(std::move(thenBranch))
        , elseBranch(std::move(elseBranch)) {
        if (this->condition) line = this->condition->line;
    }
    void accept(StmtVisitor& v) override;
};

// for (init; cond; increment) body
struct ForStmt : Stmt {
    StmtPtr  initializer; // nullable
    ExprPtr  condition;   // nullable
    ExprPtr  increment;   // nullable
    StmtPtr  body;
    ForStmt(StmtPtr initializer, ExprPtr condition, ExprPtr increment, StmtPtr body)
        : initializer(std::move(initializer))
        , condition(std::move(condition))
        , increment(std::move(increment))
        , body(std::move(body)) {
    }
    void accept(StmtVisitor& v) override;
};

// ── StmtVisitor 인터페이스 ────────────────────────────────────
class StmtVisitor {
public:
    virtual ~StmtVisitor() = default;
    virtual void visitExpressionStmt(ExpressionStmt& s) = 0;
    virtual void visitPrintStmt(PrintStmt& s) = 0;
    virtual void visitVarDeclareStmt(VarDeclareStmt& s) = 0;
    virtual void visitBlockStmt(BlockStmt& s) = 0;
    virtual void visitIfStmt(IfStmt& s) = 0;
    virtual void visitForStmt(ForStmt& s) = 0;
};

// ── accept 메서드 정의 ────────────────────────────────────────
inline void ExpressionStmt::accept(StmtVisitor& v) { v.visitExpressionStmt(*this); }
inline void PrintStmt::accept(StmtVisitor& v) { v.visitPrintStmt(*this); }
inline void VarDeclareStmt::accept(StmtVisitor& v) { v.visitVarDeclareStmt(*this); }
inline void BlockStmt::accept(StmtVisitor& v) { v.visitBlockStmt(*this); }
inline void IfStmt::accept(StmtVisitor& v) { v.visitIfStmt(*this); }
inline void ForStmt::accept(StmtVisitor& v) { v.visitForStmt(*this); }
