#include "Checker.h"

void Checker::check(const std::vector<std::unique_ptr<Stmt>>& stmts) {
    for (auto& s : stmts) checkStmt(*s);
}

void Checker::beginScope() { scopes_.push_back({}); }
void Checker::endScope() { scopes_.pop_back(); }

void Checker::declare(const Token& name) {
    if (scopes_.empty()) {
        auto it = globalScope_.find(name.lexeme);
        if (it != globalScope_.end())
            throw CheckError(name.line,
                "Variable '" + name.lexeme + "' is already declared in global scope (first declared at line " +
                std::to_string(it->second) + ").");
        globalScope_[name.lexeme] = name.line;
        return;
    }
    auto& scope = scopes_.back();
    if (scope.count(name.lexeme))
        throw CheckError(name.line, "Variable '" + name.lexeme + "' is already declared in this block.");
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
                    "Cannot read local variable '" + name.lexeme + "' in its own initializer.");
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
    try {
        for (auto& stmt : s.statements) checkStmt(*stmt);
    } catch (...) { endScope(); throw; }
    endScope();
}

void Checker::visitIfStmt(IfStmt& s) {
    checkExpr(*s.condition);
    checkStmt(*s.thenBranch);
    if (s.elseBranch) checkStmt(*s.elseBranch);
}

void Checker::visitForStmt(ForStmt& s) {
    beginScope();
    try {
        if (s.initializer) checkStmt(*s.initializer);
        if (s.condition)   checkExpr(*s.condition);
        if (s.increment)   checkExpr(*s.increment);
        checkStmt(*s.body);
    } catch (...) { endScope(); throw; }
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
    std::unordered_set<std::string> seen;
    for (const auto& param : s.params) {
        if (!seen.insert(param.lexeme).second)
            throw CheckError(param.line,
                "Duplicate parameter name '" + param.lexeme + "' in function '" + s.name.lexeme + "'.");
    }

    // Check body in a new scope with params defined
    beginScope();
    for (const auto& param : s.params)
        define(param);

    ++functionDepth_;
    try {
        for (auto& stmt : s.body)
            checkStmt(*stmt);
    } catch (...) { --functionDepth_; endScope(); throw; }
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

LiteralValue Checker::visitIndexExpr(IndexExpr& e) {
    checkExpr(*e.object);
    checkExpr(*e.index);
    return {};
}

LiteralValue Checker::visitIndexAssignExpr(IndexAssignExpr& e) {
    checkExpr(*e.target);
    checkExpr(*e.value);
    return {};
}
