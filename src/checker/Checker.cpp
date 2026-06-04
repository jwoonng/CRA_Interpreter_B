#include "Checker.h"

void Checker::check(const std::vector<std::unique_ptr<Stmt>>& stmts) {
    for (auto& s : stmts) checkStmt(*s);
}

void Checker::beginScope() {
    scopes_.push_back({});
}

void Checker::endScope() {
    scopes_.pop_back();
}

void Checker::declare(const Token& name) {
    if (scopes_.empty()) return;
    auto& scope = scopes_.back();
    if (scope.count(name.lexeme)) {
        throw CheckError(name.line,
            "변수 '" + name.lexeme + "'이(가) 이미 이 블록에서 선언되었습니다.");
    }
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
            if (!it->second) {
                throw CheckError(name.line,
                    "자신의 초기화식에서 지역변수 '" + name.lexeme + "'을(를) 읽을 수 없습니다.");
            }
            return;
        }
    }
    // 글로벌 스코프 또는 미선언 변수 — 런타임 오류는 Executor가 처리
}

void Checker::checkExpr(Expr& e) {
    if (auto* v = dynamic_cast<VariableExpr*>(&e)) {
        resolveVar(v->name);
    }
    else if (auto* b = dynamic_cast<BinaryExpr*>(&e)) {
        checkExpr(*b->left);
        checkExpr(*b->right);
    }
    else if (auto* u = dynamic_cast<UnaryExpr*>(&e)) {
        checkExpr(*u->right);
    }
    else if (auto* a = dynamic_cast<AssignExpr*>(&e)) {
        checkExpr(*a->value);
    }
    else if (auto* g = dynamic_cast<GroupingExpr*>(&e)) {
        checkExpr(*g->expression);
    }
    else if (auto* l = dynamic_cast<LogicalExpr*>(&e)) {
        checkExpr(*l->left);
        checkExpr(*l->right);
    }
    // LiteralExpr: 검사할 것 없음
}

void Checker::checkStmt(Stmt& s) {
    if (auto* decl = dynamic_cast<VarDeclareStmt*>(&s)) {
        declare(decl->name);
        if (decl->initializer) checkExpr(*decl->initializer);
        define(decl->name);
    }
    else if (auto* block = dynamic_cast<BlockStmt*>(&s)) {
        beginScope();
        for (auto& stmt : block->statements) checkStmt(*stmt);
        endScope();
    }
    else if (auto* expr = dynamic_cast<ExpressionStmt*>(&s)) {
        checkExpr(*expr->expression);
    }
    else if (auto* print = dynamic_cast<PrintStmt*>(&s)) {
        checkExpr(*print->expression);
    }
    else if (auto* ifst = dynamic_cast<IfStmt*>(&s)) {
        checkExpr(*ifst->condition);
        checkStmt(*ifst->thenBranch);
        if (ifst->elseBranch) checkStmt(*ifst->elseBranch);
    }
    else if (auto* forst = dynamic_cast<ForStmt*>(&s)) {
        beginScope();
        if (forst->initializer) checkStmt(*forst->initializer);
        if (forst->condition)   checkExpr(*forst->condition);
        if (forst->increment)   checkExpr(*forst->increment);
        checkStmt(*forst->body);
        endScope();
    }
}
