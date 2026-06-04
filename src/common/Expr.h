#pragma once
#include "Token.h"
#include <memory>
#include <vector>

// 전방 선언
class ExprVisitor;
struct LiteralExpr;
struct VariableExpr;
struct AssignExpr;
struct BinaryExpr;
struct UnaryExpr;
struct GroupingExpr;
struct LogicalExpr;

// ── Expr 기반 클래스 ──────────────────────────────────────────
struct Expr {
    int line = 0;
    virtual ~Expr() = default;
    virtual LiteralValue accept(ExprVisitor& v) = 0;
};

using ExprPtr = std::unique_ptr<Expr>;

// ── Expr 서브 클래스 ─────────────────────────────────────────
struct LiteralExpr : Expr {
    LiteralValue value;
    explicit LiteralExpr(LiteralValue value, int line = 0) : value(std::move(value)) { this->line = line; }
    LiteralValue accept(ExprVisitor& v) override;
};

struct VariableExpr : Expr {
    Token name;
    explicit VariableExpr(Token name) : name(std::move(name)) { this->line = this->name.line; }
    LiteralValue accept(ExprVisitor& v) override;
};

struct AssignExpr : Expr {
    Token name;
    ExprPtr value;
    AssignExpr(Token name, ExprPtr value) : name(std::move(name)), value(std::move(value)) { this->line = this->name.line; }
    LiteralValue accept(ExprVisitor& v) override;
};

struct BinaryExpr : Expr {
    ExprPtr left;
    Token op;
    ExprPtr right;
    BinaryExpr(ExprPtr left, Token op, ExprPtr right)
        : left(std::move(left)), op(std::move(op)), right(std::move(right)) {
        this->line = this->op.line;
    }
    LiteralValue accept(ExprVisitor& v) override;
};

struct UnaryExpr : Expr {
    Token op;
    ExprPtr right;
    UnaryExpr(Token op, ExprPtr right) : op(std::move(op)), right(std::move(right)) { this->line = this->op.line; }
    LiteralValue accept(ExprVisitor& v) override;
};

struct GroupingExpr : Expr {
    ExprPtr expression;
    explicit GroupingExpr(ExprPtr expression) : expression(std::move(expression)) {
        if (this->expression) this->line = this->expression->line;
    }
    LiteralValue accept(ExprVisitor& v) override;
};

struct LogicalExpr : Expr {
    ExprPtr left;
    Token op;
    ExprPtr right;
    LogicalExpr(ExprPtr left, Token op, ExprPtr right)
        : left(std::move(left)), op(std::move(op)), right(std::move(right)) {
        this->line = this->op.line;
    }
    LiteralValue accept(ExprVisitor& v) override;
};

// ── ExprVisitor 인터페이스 ────────────────────────────────────
class ExprVisitor {
public:
    virtual ~ExprVisitor() = default;
    virtual LiteralValue visitLiteralExpr(LiteralExpr& e) = 0;
    virtual LiteralValue visitVariableExpr(VariableExpr& e) = 0;
    virtual LiteralValue visitAssignExpr(AssignExpr& e) = 0;
    virtual LiteralValue visitBinaryExpr(BinaryExpr& e) = 0;
    virtual LiteralValue visitUnaryExpr(UnaryExpr& e) = 0;
    virtual LiteralValue visitGroupingExpr(GroupingExpr& e) = 0;
    virtual LiteralValue visitLogicalExpr(LogicalExpr& e) = 0;
};

// ── accept 메서드 정의 (ExprVisitor 완성 이후) ────────────────
inline LiteralValue LiteralExpr::accept(ExprVisitor& v) { return v.visitLiteralExpr(*this); }
inline LiteralValue VariableExpr::accept(ExprVisitor& v) { return v.visitVariableExpr(*this); }
inline LiteralValue AssignExpr::accept(ExprVisitor& v) { return v.visitAssignExpr(*this); }
inline LiteralValue BinaryExpr::accept(ExprVisitor& v) { return v.visitBinaryExpr(*this); }
inline LiteralValue UnaryExpr::accept(ExprVisitor& v) { return v.visitUnaryExpr(*this); }
inline LiteralValue GroupingExpr::accept(ExprVisitor& v) { return v.visitGroupingExpr(*this); }
inline LiteralValue LogicalExpr::accept(ExprVisitor& v) { return v.visitLogicalExpr(*this); }
