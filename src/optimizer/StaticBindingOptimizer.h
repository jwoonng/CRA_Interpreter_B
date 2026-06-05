#pragma once
#include "IOptimizer.h"
#include <unordered_map>

// ── Resolver ──────────────────────────────────────────────────────────────
// AST를 순회하며 VariableExpr / AssignExpr 의 distance 필드를 설정한다.
//
//   distance == -1 : 글로벌 변수 또는 미최적화 → Executor가 동적 탐색(기존 방식)
//   distance >= 0  : 로컬 변수 → Executor가 getAt(distance) 로 O(1) 직접 접근
//
// Checker와 동일한 스코프 스택 구조를 사용하지만 오류 검사는 하지 않는다.
class Resolver : private ExprVisitor, private StmtVisitor {
public:
    void resolve(const std::vector<std::unique_ptr<Stmt>>& stmts) {
        for (auto& s : stmts) resolveStmt(*s);
    }

private:
    // key = 변수명, value = 초기화 완료 여부 (Checker 방식 그대로 유지)
    std::vector<std::unordered_map<std::string, bool>> scopes_;

    // ── 스코프 관리 ───────────────────────────────────────────────────────
    void beginScope() { scopes_.push_back({}); }
    void endScope()   { scopes_.pop_back(); }

    void declare(const Token& t) {
        if (!scopes_.empty()) scopes_.back()[t.lexeme] = false;
    }
    void define(const Token& t) {
        if (!scopes_.empty()) scopes_.back()[t.lexeme] = true;
    }

    // ── distance 계산 ─────────────────────────────────────────────────────
    // 현재 스코프(scopes_.back())에서 선언 스코프까지의 거리를 반환한다.
    // 로컬 스코프에 없으면 -1 반환 (글로벌 → Executor 동적 탐색 fallback)
    int computeDistance(const std::string& name) const {
        for (int i = static_cast<int>(scopes_.size()) - 1; i >= 0; --i)
            if (scopes_[i].count(name))
                return static_cast<int>(scopes_.size()) - 1 - i;
        return -1;
    }

    void resolveLocal(VariableExpr& e) {
        int d = computeDistance(e.name.lexeme);
        if (d >= 0) e.distance = d;
    }

    void resolveLocalAssign(AssignExpr& e) {
        int d = computeDistance(e.name.lexeme);
        if (d >= 0) e.distance = d;
    }

    void resolveStmt(Stmt& s) { s.accept(*this); }
    void resolveExpr(Expr& e) { e.accept(*this); }

    // ── StmtVisitor 구현 ──────────────────────────────────────────────────
    void visitExpressionStmt(ExpressionStmt& s) override { resolveExpr(*s.expression); }
    void visitPrintStmt(PrintStmt& s)           override { resolveExpr(*s.expression); }

    void visitVarDeclareStmt(VarDeclareStmt& s) override {
        declare(s.name);
        if (s.initializer) resolveExpr(*s.initializer);
        define(s.name);
    }

    void visitBlockStmt(BlockStmt& s) override {
        beginScope();
        for (auto& stmt : s.statements) resolveStmt(*stmt);
        endScope();
    }

    void visitIfStmt(IfStmt& s) override {
        resolveExpr(*s.condition);
        resolveStmt(*s.thenBranch);
        if (s.elseBranch) resolveStmt(*s.elseBranch);
    }

    void visitForStmt(ForStmt& s) override {
        beginScope();
        if (s.initializer) resolveStmt(*s.initializer);
        if (s.condition)   resolveExpr(*s.condition);
        if (s.increment)   resolveExpr(*s.increment);
        resolveStmt(*s.body);
        endScope();
    }

    // 함수 선언: 함수명 등록 후 파라미터를 새 스코프에 선언
    void visitFunctionStmt(FunctionStmt& s) override {
        declare(s.name);
        define(s.name);
        beginScope();
        for (const auto& p : s.params) { declare(p); define(p); }
        for (auto& stmt : s.body) resolveStmt(*stmt);
        endScope();
    }

    void visitReturnStmt(ReturnStmt& s) override {
        if (s.value) resolveExpr(*s.value);
    }

    // ── ExprVisitor 구현 ──────────────────────────────────────────────────
    LiteralValue visitLiteralExpr(LiteralExpr&)     override { return {}; }
    LiteralValue visitGroupingExpr(GroupingExpr& e) override { resolveExpr(*e.expression); return {}; }
    LiteralValue visitUnaryExpr(UnaryExpr& e)       override { resolveExpr(*e.right);      return {}; }

    LiteralValue visitBinaryExpr(BinaryExpr& e) override {
        resolveExpr(*e.left);
        resolveExpr(*e.right);
        return {};
    }
    LiteralValue visitLogicalExpr(LogicalExpr& e) override {
        resolveExpr(*e.left);
        resolveExpr(*e.right);
        return {};
    }

    // VariableExpr: distance 설정
    LiteralValue visitVariableExpr(VariableExpr& e) override {
        resolveLocal(e);
        return {};
    }

    // AssignExpr: RHS 먼저 처리 후 LHS distance 설정
    LiteralValue visitAssignExpr(AssignExpr& e) override {
        resolveExpr(*e.value);
        resolveLocalAssign(e);
        return {};
    }

    LiteralValue visitCallExpr(CallExpr& e) override {
        resolveExpr(*e.callee);
        for (auto& arg : e.arguments) resolveExpr(*arg);
        return {};
    }

    LiteralValue visitIndexExpr(IndexExpr& e) override {
        resolveExpr(*e.object);
        resolveExpr(*e.index);
        return {};
    }

    LiteralValue visitIndexAssignExpr(IndexAssignExpr& e) override {
        resolveExpr(*e.target->object);
        resolveExpr(*e.target->index);
        resolveExpr(*e.value);
        return {};
    }
};

// ── StaticBindingOptimizer ─────────────────────────────────────────────────
// IOptimizer 구현 (Strategy Pattern)
// Shell::addOptimizer() 로 파이프라인에 추가하면 Checker 이후 자동 실행된다.
//
//   사용 예:
//     shell.addOptimizer(std::make_unique<StaticBindingOptimizer>());
struct StaticBindingOptimizer : IOptimizer {
    std::vector<std::unique_ptr<Stmt>> optimize(
        std::vector<std::unique_ptr<Stmt>> stmts) override {
        Resolver resolver;
        resolver.resolve(stmts);   // 노드를 in-place 변환 (distance 설정)
        return stmts;
    }
};
