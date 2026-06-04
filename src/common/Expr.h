#pragma once
#include "Token.h"
#include <memory>
#include <stdexcept>
#include <vector>

// 전방 선언 — 기존 노드
class ExprVisitor;
struct LiteralExpr;
struct VariableExpr;
struct AssignExpr;
struct BinaryExpr;
struct UnaryExpr;
struct GroupingExpr;
struct LogicalExpr;

// 전방 선언 — 확장 노드 (함수 호출 / 배열)
struct CallExpr;
struct ArrayLiteralExpr;
struct IndexExpr;

// ── Expr 기반 클래스 ──────────────────────────────────────────
struct Expr {
    int line = 0;
    virtual ~Expr() = default;
    virtual LiteralValue accept(ExprVisitor& v) = 0;
};

using ExprPtr = std::unique_ptr<Expr>;

// ── Expr 서브 클래스 (기존) ───────────────────────────────────
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

// ── Expr 서브 클래스 (확장) ───────────────────────────────────

// 함수 호출: callee(arg1, arg2, ...)
struct CallExpr : Expr {
    ExprPtr              callee;
    Token                paren;       // ')' 위치 (오류 보고용)
    std::vector<ExprPtr> arguments;
    CallExpr(ExprPtr callee, Token paren, std::vector<ExprPtr> arguments)
        : callee(std::move(callee))
        , paren(std::move(paren))
        , arguments(std::move(arguments)) {
        line = this->paren.line;
    }
    LiteralValue accept(ExprVisitor& v) override;
};

// 배열 리터럴: [elem1, elem2, ...]
struct ArrayLiteralExpr : Expr {
    std::vector<ExprPtr> elements;
    explicit ArrayLiteralExpr(std::vector<ExprPtr> elements, int line = 0)
        : elements(std::move(elements)) {
        this->line = line;
    }
    LiteralValue accept(ExprVisitor& v) override;
};

// 인덱스 접근: object[index]
struct IndexExpr : Expr {
    ExprPtr object;
    ExprPtr index;
    Token   bracket;  // ']' 위치 (오류 보고용)
    IndexExpr(ExprPtr object, ExprPtr index, Token bracket)
        : object(std::move(object))
        , index(std::move(index))
        , bracket(std::move(bracket)) {
        line = this->bracket.line;
    }
    LiteralValue accept(ExprVisitor& v) override;
};

// ── ExprVisitor 인터페이스 ────────────────────────────────────
class ExprVisitor {
public:
    virtual ~ExprVisitor() = default;

    // 기존 — 순수 가상 (모든 구현체 필수)
    virtual LiteralValue visitLiteralExpr(LiteralExpr& e) = 0;
    virtual LiteralValue visitVariableExpr(VariableExpr& e) = 0;
    virtual LiteralValue visitAssignExpr(AssignExpr& e) = 0;
    virtual LiteralValue visitBinaryExpr(BinaryExpr& e) = 0;
    virtual LiteralValue visitUnaryExpr(UnaryExpr& e) = 0;
    virtual LiteralValue visitGroupingExpr(GroupingExpr& e) = 0;
    virtual LiteralValue visitLogicalExpr(LogicalExpr& e) = 0;

    // 확장 — 기본 throw 구현 (기능 구현 시 override)
    virtual LiteralValue visitCallExpr(CallExpr& e) {
        throw std::runtime_error("[line " + std::to_string(e.line) + "] Not implemented: function call");
    }
    virtual LiteralValue visitArrayLiteralExpr(ArrayLiteralExpr& e) {
        throw std::runtime_error("[line " + std::to_string(e.line) + "] Not implemented: array literal");
    }
    virtual LiteralValue visitIndexExpr(IndexExpr& e) {
        throw std::runtime_error("[line " + std::to_string(e.line) + "] Not implemented: index expression");
    }
};

// ── accept 메서드 정의 ────────────────────────────────────────
inline LiteralValue LiteralExpr::accept(ExprVisitor& v)      { return v.visitLiteralExpr(*this); }
inline LiteralValue VariableExpr::accept(ExprVisitor& v)     { return v.visitVariableExpr(*this); }
inline LiteralValue AssignExpr::accept(ExprVisitor& v)       { return v.visitAssignExpr(*this); }
inline LiteralValue BinaryExpr::accept(ExprVisitor& v)       { return v.visitBinaryExpr(*this); }
inline LiteralValue UnaryExpr::accept(ExprVisitor& v)        { return v.visitUnaryExpr(*this); }
inline LiteralValue GroupingExpr::accept(ExprVisitor& v)     { return v.visitGroupingExpr(*this); }
inline LiteralValue LogicalExpr::accept(ExprVisitor& v)      { return v.visitLogicalExpr(*this); }
inline LiteralValue CallExpr::accept(ExprVisitor& v)         { return v.visitCallExpr(*this); }
inline LiteralValue ArrayLiteralExpr::accept(ExprVisitor& v) { return v.visitArrayLiteralExpr(*this); }
inline LiteralValue IndexExpr::accept(ExprVisitor& v)        { return v.visitIndexExpr(*this); }
