#include "Checker.h"

void Checker::check(const std::vector<std::unique_ptr<Stmt>>& stmts) {
    for (auto& s : stmts) checkStmt(*s);
}

void Checker::beginScope() { scopes_.push_back({}); }
void Checker::endScope() { scopes_.pop_back(); }

void Checker::declare(const Token& name) {
    if (scopes_.empty()) return;
    auto& scope = scopes_.back();
    if (scope.count(name.lexeme))
        throw CheckError(name.line, "변수 '" + name.lexeme + "'이(가) 이미 이 블록에서 선언되었습니다.");
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
    // 글로벌 또는 미선언 변수 — 런타임 오류는 Executor가 처리
}

void Checker::checkStmt(Stmt& s) { s.accept(*this); }
void Checker::checkExpr(Expr& e) { e.accept(*this); }

// ── StmtVisitor 구현 ──────────────────────────────────────────

void Checker::visitExpressionStmt(ExpressionStmt& s) {
    checkExpr(*s.expression);
}

void Checker::visitPrintStmt(PrintStmt& s) {
    checkExpr(*s.expression);
}

void Checker::visitVarDeclareStmt(VarDeclareStmt& s) {
    declare(s.name);
    if (s.initializer) checkExpr(*s.initializer);
    define(s.name);
}

void Checker::visitBlockStmt(BlockStmt& s) {
    beginScope();
    for (auto& stmt : s.statements) checkStmt(*stmt);
    endScope();
}

void Checker::visitIfStmt(IfStmt& s) {
    checkExpr(*s.condition);
    checkStmt(*s.thenBranch);
    if (s.elseBranch) checkStmt(*s.elseBranch);
}

void Checker::visitForStmt(ForStmt& s) {
    beginScope();
    if (s.initializer) checkStmt(*s.initializer);
    if (s.condition)   checkExpr(*s.condition);
    if (s.increment)   checkExpr(*s.increment);
    checkStmt(*s.body);
    endScope();
}

// ── ExprVisitor 구현 ─────────────────────────────────────────

LiteralValue Checker::visitLiteralExpr(LiteralExpr&) { return {}; }

LiteralValue Checker::visitVariableExpr(VariableExpr& e) {
    resolveVar(e.name);
    return {};
}

LiteralValue Checker::visitAssignExpr(AssignExpr& e) {
    checkExpr(*e.value);
    // 할당 대상 글로벌 변수는 Executor에서 처리
    return {};
}

LiteralValue Checker::visitBinaryExpr(BinaryExpr& e) {
    checkExpr(*e.left);
    checkExpr(*e.right);
    return {};
}

LiteralValue Checker::visitUnaryExpr(UnaryExpr& e) {
    checkExpr(*e.right);
    return {};
}

LiteralValue Checker::visitGroupingExpr(GroupingExpr& e) {
    checkExpr(*e.expression);
    return {};
}

LiteralValue Checker::visitLogicalExpr(LogicalExpr& e) {
    checkExpr(*e.left);
    checkExpr(*e.right);
    return {};
}

void Checker::visitFunctionStmt(FunctionStmt& s) {
    // Check for duplicate parameter names
    std::unordered_set<std::string> seen;
    for (const auto& param : s.params) {
        if (seen.count(param.lexeme))
            throw CheckError(param.line,
                "Duplicate parameter name '" + param.lexeme + "' in function '" + s.name.lexeme + "'.");
        seen.insert(param.lexeme);
    }

    // Check body in a new scope with params defined
    beginScope();
    for (const auto& param : s.params)
        define(param);

    ++functionDepth_;
    for (auto& stmt : s.body)
        checkStmt(*stmt);
    --functionDepth_;

    endScope();
}

void Checker::visitReturnStmt(ReturnStmt& s) {
    if (functionDepth_ == 0)
        throw CheckError(s.keyword.line,
            "Cannot use 'return' outside of a function.");
    if (s.value)
        checkExpr(*s.value);
}

LiteralValue Checker::visitCallExpr(CallExpr& e) {
    checkExpr(*e.callee);
    for (auto& arg : e.arguments)
        checkExpr(*arg);
    return {};
}
