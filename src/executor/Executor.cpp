#include "Executor.h"
#include <cmath>
#include <limits>
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

// ── 내부 헬퍼 ───────────────────────────────────────────────────
static std::pair<double, double> numericOperands(const LiteralValue& l,
                                                 const LiteralValue& r) {
    auto* ld = std::get_if<double>(&l);
    auto* rd = std::get_if<double>(&r);
    if (!ld || !rd) throw std::runtime_error("Operands must be numbers.");
    return {*ld, *rd};
}

static std::string formatDouble(double d) {
    constexpr double LLONG_MIN_D = static_cast<double>(std::numeric_limits<long long>::min());
    constexpr double LLONG_MAX_D = static_cast<double>(std::numeric_limits<long long>::max());
    if (d == std::floor(d) && d >= LLONG_MIN_D && d <= LLONG_MAX_D)
        return std::to_string(static_cast<long long>(d));
    std::ostringstream oss;
    oss << d;
    return oss.str();
}

// ── 출력 포맷 ────────────────────────────────────────────────────
std::string Executor::stringify(const LiteralValue& v) {
    return std::visit([](auto&& val) -> std::string {
        using T = std::decay_t<decltype(val)>;
        if constexpr (std::is_same_v<T, std::monostate>) return "nil";
        if constexpr (std::is_same_v<T, bool>)           return val ? "true" : "false";
        if constexpr (std::is_same_v<T, std::string>)    return val;
        if constexpr (std::is_same_v<T, double>)         return formatDouble(val);
    }, v);
}

// ── ExprVisitor ──────────────────────────────────────────────────
LiteralValue Executor::visitLiteralExpr(LiteralExpr& e) {
    return e.value;
}

LiteralValue Executor::visitVariableExpr(VariableExpr& e) {
    return env_->get(e.name.lexeme);
}

LiteralValue Executor::visitBinaryExpr(BinaryExpr& e) {
    LiteralValue left  = evaluate(*e.left);
    LiteralValue right = evaluate(*e.right);

    switch (e.op.type) {
        case TokenType::PLUS: { auto [l, r] = numericOperands(left, right); return l + r; }
        case TokenType::STAR: { auto [l, r] = numericOperands(left, right); return l * r; }
        default: break;
    }
    return std::monostate{};
}

LiteralValue Executor::visitAssignExpr(AssignExpr&)     { return std::monostate{}; }
LiteralValue Executor::visitUnaryExpr(UnaryExpr&)       { return std::monostate{}; }
LiteralValue Executor::visitGroupingExpr(GroupingExpr&) { return std::monostate{}; }
LiteralValue Executor::visitLogicalExpr(LogicalExpr&)   { return std::monostate{}; }

// ── StmtVisitor ──────────────────────────────────────────────────
void Executor::visitPrintStmt(PrintStmt& s) {
    *out_ << stringify(evaluate(*s.expression)) << "\n";
}

void Executor::visitVarDeclareStmt(VarDeclareStmt& s) {
    LiteralValue val = s.initializer ? evaluate(*s.initializer) : LiteralValue{};
    env_->define(s.name.lexeme, std::move(val));
}

void Executor::visitExpressionStmt(ExpressionStmt&) {}
void Executor::visitBlockStmt(BlockStmt&)           {}
void Executor::visitIfStmt(IfStmt&)                 {}
void Executor::visitForStmt(ForStmt&)               {}
