#include "Checker.h"

void Checker::check(const std::vector<std::unique_ptr<Stmt>>& stmts) {
    for (auto& s : stmts) s->accept(*this);
}

// ── 스코프 관리 ───────────────────────────────────────────────────────────────

void Checker::beginScope() { scopes_.push_back({}); }
void Checker::endScope()   { scopes_.pop_back(); }

void Checker::declare(const Token& name) {
    if (scopes_.empty()) return;
    auto& scope = scopes_.back();
    if (scope.count(name.lexeme))
        throw CheckError(name.line,
            "변수 '" + name.lexeme + "'이(가) 이미 이 블록에서 선언되었습니다.");
    scope[name.lexeme] = false;
}

void Checker::define(const Token& name) {
    if (scopes_.empty()) return;
    scopes_.back()[name.lexeme] = true;
}

void Checker::resolveVar(const Token& name) {
    for (int i = static_cast<int>(scopes_.size()) - 1; i >= 0; --i) {
        auto it = scopes_[i].find(name.lexeme);
        if (it != scopes_[i].end()) {
            if (!it->second)
                throw CheckError(name.line,
                    "자신의 초기화식에서 지역변수 '" + name.lexeme + "'을(를) 읽을 수 없습니다.");
            return;
        }
    }
    // 글로벌 스코프 또는 미선언 변수 — 런타임 오류는 Executor가 처리
}

// ── StmtVisitor 구현 ──────────────────────────────────────────────────────────

void Checker::visitVarDeclareStmt(VarDeclareStmt& s) {
    declare(s.name);
    if (s.initializer) s.initializer->accept(*this);
    define(s.name);
}

void Checker::visitBlockStmt(BlockStmt& s) {
    beginScope();
    for (auto& stmt : s.statements) stmt->accept(*this);
    endScope();
}

void Checker::visitExpressionStmt(ExpressionStmt& s) {
    s.expression->accept(*this);
}

void Checker::visitPrintStmt(PrintStmt& s) {
    s.expression->accept(*this);
}

void Checker::visitIfStmt(IfStmt& s) {
    s.condition->accept(*this);
    s.thenBranch->accept(*this);
    if (s.elseBranch) s.elseBranch->accept(*this);
}

void Checker::visitForStmt(ForStmt& s) {
    beginScope();
    if (s.initializer) s.initializer->accept(*this);
    if (s.condition)   s.condition->accept(*this);
    if (s.increment)   s.increment->accept(*this);
    s.body->accept(*this);
    endScope();
}

// ── ExprVisitor 구현 ──────────────────────────────────────────────────────────

LiteralValue Checker::visitLiteralExpr(LiteralExpr&) { return {}; }

LiteralValue Checker::visitVariableExpr(VariableExpr& e) {
    resolveVar(e.name);
    return {};
}

LiteralValue Checker::visitAssignExpr(AssignExpr& e) {
    resolveVar(e.name);      // 대입 대상 변수가 선언되어 있는지 확인
    e.value->accept(*this);
    return {};
}

LiteralValue Checker::visitBinaryExpr(BinaryExpr& e) {
    e.left->accept(*this);
    e.right->accept(*this);
    return {};
}

LiteralValue Checker::visitUnaryExpr(UnaryExpr& e) {
    e.right->accept(*this);
    return {};
}

LiteralValue Checker::visitGroupingExpr(GroupingExpr& e) {
    e.expression->accept(*this);
    return {};
}

LiteralValue Checker::visitLogicalExpr(LogicalExpr& e) {
    e.left->accept(*this);
    e.right->accept(*this);
    return {};
}
