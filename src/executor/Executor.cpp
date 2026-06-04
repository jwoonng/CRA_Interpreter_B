#include "Executor.h"
#include <cmath>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <utility>

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

static bool isTruthy(const LiteralValue& v) {
    if (auto* b = std::get_if<bool>(&v))   return *b;
    if (auto* d = std::get_if<double>(&v)) return *d != 0.0;
    if (std::get_if<std::monostate>(&v))   return false;
    return true;
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
        case TokenType::PLUS: {
            auto* ls = std::get_if<std::string>(&left);
            auto* rs = std::get_if<std::string>(&right);
            if (ls && rs) return *ls + *rs;
            auto [l, r] = numericOperands(left, right);
            return l + r;
        }
        case TokenType::MINUS: { auto [l, r] = numericOperands(left, right); return l - r; }
        case TokenType::STAR:  { auto [l, r] = numericOperands(left, right); return l * r; }
        case TokenType::SLASH: {
            auto [l, r] = numericOperands(left, right);
            if (r == 0.0) throw std::runtime_error("Division by zero.");
            return l / r;
        }
        case TokenType::EQUAL_EQUAL:   return left == right;
        case TokenType::BANG_EQUAL:    return left != right;
        case TokenType::GREATER:       { auto [l, r] = numericOperands(left, right); return l > r; }
        case TokenType::GREATER_EQUAL: { auto [l, r] = numericOperands(left, right); return l >= r; }
        case TokenType::LESS:          { auto [l, r] = numericOperands(left, right); return l < r; }
        case TokenType::LESS_EQUAL:    { auto [l, r] = numericOperands(left, right); return l <= r; }
        default: break;
    }
    return std::monostate{};
}

LiteralValue Executor::visitAssignExpr(AssignExpr& e) {
    LiteralValue val = evaluate(*e.value);
    env_->assign(e.name.lexeme, val);
    return val;
}

LiteralValue Executor::visitUnaryExpr(UnaryExpr& e) {
    LiteralValue right = evaluate(*e.right);
    if (e.op.type == TokenType::BANG)  return !isTruthy(right);
    if (e.op.type == TokenType::MINUS) {
        auto* d = std::get_if<double>(&right);
        if (!d) throw std::runtime_error("Operand must be a number.");
        return -*d;
    }
    return std::monostate{};
}

LiteralValue Executor::visitGroupingExpr(GroupingExpr& e) {
    return evaluate(*e.expression);
}

LiteralValue Executor::visitLogicalExpr(LogicalExpr& e) {
    LiteralValue left = evaluate(*e.left);
    if (e.op.type == TokenType::OR)
        return isTruthy(left) ? left : evaluate(*e.right);
    return !isTruthy(left) ? left : evaluate(*e.right);
}

// ── StmtVisitor ──────────────────────────────────────────────────
void Executor::visitPrintStmt(PrintStmt& s) {
    *out_ << stringify(evaluate(*s.expression)) << "\n";
}

void Executor::visitVarDeclareStmt(VarDeclareStmt& s) {
    LiteralValue val = s.initializer ? evaluate(*s.initializer) : LiteralValue{};
    env_->define(s.name.lexeme, std::move(val));
}

void Executor::visitExpressionStmt(ExpressionStmt& s) {
    evaluate(*s.expression);
}

void Executor::visitBlockStmt(BlockStmt& s) {
    Environment local(env_);
    struct Guard { Environment*& e; Environment* p; ~Guard() { e = p; } }
        g{env_, std::exchange(env_, &local)};
    for (auto& stmt : s.statements) run(*stmt);
}

void Executor::visitIfStmt(IfStmt& s) {
    if (isTruthy(evaluate(*s.condition)))
        run(*s.thenBranch);
    else if (s.elseBranch)
        run(*s.elseBranch);
}

void Executor::visitForStmt(ForStmt& s) {
    Environment forScope(env_);
    struct Guard { Environment*& e; Environment* p; ~Guard() { e = p; } }
        g{env_, std::exchange(env_, &forScope)};
    if (s.initializer) run(*s.initializer);
    while (true) {
        if (s.condition && !isTruthy(evaluate(*s.condition))) break;
        run(*s.body);
        if (s.increment) evaluate(*s.increment);
    }
}
