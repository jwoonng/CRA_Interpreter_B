#include "Executor.h"
#include <sstream>
#include <stdexcept>

// ── 진입점 ───────────────────────────────────────────────────────
void Executor::execute(const std::vector<std::unique_ptr<Stmt>>& stmts,
                       std::ostream& out) {
    out_ = &out;
    for (auto& s : stmts) run(*s);
}

LiteralValue Executor::evaluate(Expr& e) { return e.accept(*this); }
void         Executor::run(Stmt& s)      { s.accept(*this); }

// ── 출력 포맷 ────────────────────────────────────────────────────
std::string Executor::stringify(const LiteralValue& v) {
    double d = std::get<double>(v);
    if (d == static_cast<long long>(d))
        return std::to_string(static_cast<long long>(d));
    std::ostringstream oss;
    oss << d;
    return oss.str();
}

// ── ExprVisitor ──────────────────────────────────────────────────
LiteralValue Executor::visitLiteralExpr(LiteralExpr& e) {
    return e.value;
}

LiteralValue Executor::visitVariableExpr(VariableExpr& e) {
    return env_->get(e.name.lexeme);
}

LiteralValue Executor::visitAssignExpr(AssignExpr& e) {
    return std::monostate{};
}

LiteralValue Executor::visitBinaryExpr(BinaryExpr& e) {
    LiteralValue left  = evaluate(*e.left);
    LiteralValue right = evaluate(*e.right);

    switch (e.op.type) {
        case TokenType::PLUS:
            if (auto* l = std::get_if<double>(&left))
                if (auto* r = std::get_if<double>(&right)) return *l + *r;
            break;
        case TokenType::STAR:
            if (auto* l = std::get_if<double>(&left))
                if (auto* r = std::get_if<double>(&right)) return *l * *r;
            break;
        default: break;
    }
    return std::monostate{};
}

LiteralValue Executor::visitUnaryExpr(UnaryExpr& e)       { return std::monostate{}; }
LiteralValue Executor::visitGroupingExpr(GroupingExpr& e) { return std::monostate{}; }
LiteralValue Executor::visitLogicalExpr(LogicalExpr& e)   { return std::monostate{}; }

// ── StmtVisitor ──────────────────────────────────────────────────
void Executor::visitExpressionStmt(ExpressionStmt& s) {}

void Executor::visitPrintStmt(PrintStmt& s) {
    *out_ << stringify(evaluate(*s.expression)) << "\n";
}

void Executor::visitVarDeclareStmt(VarDeclareStmt& s) {
    LiteralValue val = s.initializer ? evaluate(*s.initializer) : LiteralValue{};
    env_->define(s.name.lexeme, std::move(val));
}

void Executor::visitBlockStmt(BlockStmt& s) {}
void Executor::visitIfStmt(IfStmt& s)       {}
void Executor::visitForStmt(ForStmt& s)     {}
