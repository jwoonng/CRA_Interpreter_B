#include "Executor.h"
#include <cmath>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <utility>

// ── 진입점 ───────────────────────────────────────────────────────
void Executor::execute(std::vector<std::unique_ptr<Stmt>> stmts,
                       std::ostream& out) {
    out_ = &out;
    ownedStmts_.push_back(std::move(stmts));
    for (auto& s : ownedStmts_.back()) run(*s);
}

LiteralValue Executor::evaluate(Expr& e) { return e.accept(*this); }
void         Executor::run(Stmt& s)      { s.accept(*this); }

// ── 내부 헬퍼 ───────────────────────────────────────────────────
static std::pair<double, double> numericOperands(const LiteralValue& l,
                                                 const LiteralValue& r,
                                                 int line) {
    auto* ld = std::get_if<double>(&l);
    auto* rd = std::get_if<double>(&r);
    if (!ld || !rd)
        throw std::runtime_error("[line " + std::to_string(line) + "] Operands must be numbers.");
    return {*ld, *rd};
}

static bool isTruthy(const LiteralValue& v) {
    if (auto* b = std::get_if<bool>(&v))   return *b;
    if (auto* d = std::get_if<double>(&v)) return *d != 0.0;
    if (std::get_if<std::monostate>(&v))   return false;
    return true;
}

struct ScopeGuard {
    Environment*& ref;
    Environment*  prev;
    ~ScopeGuard() { ref = prev; }
};

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
    return std::visit([this](auto&& val) -> std::string {
        using T = std::decay_t<decltype(val)>;
        if constexpr (std::is_same_v<T, std::monostate>) return "nil";
        if constexpr (std::is_same_v<T, bool>)           return val ? "true" : "false";
        if constexpr (std::is_same_v<T, std::string>)    return val;
        if constexpr (std::is_same_v<T, double>)         return formatDouble(val);
        if constexpr (std::is_same_v<T, ArrayPtr>) {
            if (!val) return "nil";
            std::string result = "[";
            for (std::size_t i = 0; i < val->elements.size(); ++i) {
                if (i > 0) result += ", ";
                result += stringify(val->elements[i]);
            }
            return result + "]";
        }
    }, v);
}

// ── ExprVisitor ──────────────────────────────────────────────────
LiteralValue Executor::visitLiteralExpr(LiteralExpr& e) {
    return e.value;
}

LiteralValue Executor::visitVariableExpr(VariableExpr& e) {
    if (e.distance >= 0)
        return env_->getAt(e.distance, e.name.lexeme);   // O(1): StaticBindingOptimizer가 설정
    return env_->get(e.name.lexeme, e.name.line);         // fallback: 글로벌 또는 미최적화
}

LiteralValue Executor::visitBinaryExpr(BinaryExpr& e) {
    LiteralValue left  = evaluate(*e.left);
    LiteralValue right = evaluate(*e.right);

    switch (e.op.type) {
        case TokenType::PLUS: {
            auto* ls = std::get_if<std::string>(&left);
            auto* rs = std::get_if<std::string>(&right);
            if (ls && rs) return *ls + *rs;
            auto [l, r] = numericOperands(left, right, e.op.line);
            return l + r;
        }
        case TokenType::MINUS: { auto [l, r] = numericOperands(left, right, e.op.line); return l - r; }
        case TokenType::STAR:  { auto [l, r] = numericOperands(left, right, e.op.line); return l * r; }
        case TokenType::SLASH: {
            auto [l, r] = numericOperands(left, right, e.op.line);
            if (r == 0.0) throw std::runtime_error("[line " + std::to_string(e.op.line) + "] Division by zero.");
            return l / r;
        }
        case TokenType::PERCENT: {
            auto [l, r] = numericOperands(left, right, e.op.line);
            if (r == 0.0) throw std::runtime_error("[line " + std::to_string(e.op.line) + "] Division by zero.");
            return std::fmod(l, r);
        }
        case TokenType::EQUAL_EQUAL:   return left == right;
        case TokenType::BANG_EQUAL:    return left != right;
        case TokenType::GREATER:       { auto [l, r] = numericOperands(left, right, e.op.line); return l > r; }
        case TokenType::GREATER_EQUAL: { auto [l, r] = numericOperands(left, right, e.op.line); return l >= r; }
        case TokenType::LESS:          { auto [l, r] = numericOperands(left, right, e.op.line); return l < r; }
        case TokenType::LESS_EQUAL:    { auto [l, r] = numericOperands(left, right, e.op.line); return l <= r; }
        default: throw std::runtime_error("[line " + std::to_string(e.op.line) + "] Unimplemented operator: " + e.op.lexeme);
    }
}

LiteralValue Executor::visitAssignExpr(AssignExpr& e) {
    LiteralValue val = evaluate(*e.value);
    if (e.distance >= 0)
        env_->assignAt(e.distance, e.name.lexeme, val);  // O(1): StaticBindingOptimizer가 설정
    else
        env_->assign(e.name.lexeme, val, e.name.line);   // fallback: 글로벌 또는 미최적화
    return val;
}

LiteralValue Executor::visitUnaryExpr(UnaryExpr& e) {
    LiteralValue right = evaluate(*e.right);
    switch (e.op.type) {
        case TokenType::BANG:  return !isTruthy(right);
        case TokenType::MINUS: {
            auto* d = std::get_if<double>(&right);
            if (!d) throw std::runtime_error("[line " + std::to_string(e.op.line) + "] Operand must be a number.");
            return -*d;
        }
        default: throw std::runtime_error("[line " + std::to_string(e.op.line) + "] Unimplemented operator: " + e.op.lexeme);
    }
}

LiteralValue Executor::visitGroupingExpr(GroupingExpr& e) {
    return evaluate(*e.expression);
}

LiteralValue Executor::visitLogicalExpr(LogicalExpr& e) {
    LiteralValue left = evaluate(*e.left);
    if (e.op.type == TokenType::OR)
        return isTruthy(left) ? left : evaluate(*e.right);
    if (e.op.type == TokenType::AND)
        return !isTruthy(left) ? left : evaluate(*e.right);
    throw std::runtime_error("[line " + std::to_string(e.op.line) + "] Unimplemented operator: " + e.op.lexeme);
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
    ScopeGuard g{env_, std::exchange(env_, &local)};
    for (auto& stmt : s.statements) run(*stmt);
}

void Executor::visitIfStmt(IfStmt& s) {
    if (isTruthy(evaluate(*s.condition)))
        run(*s.thenBranch);
    else if (s.elseBranch)
        run(*s.elseBranch);
}

void Executor::visitFunctionStmt(FunctionStmt& s) {
    functions_[s.name.lexeme] = FunctionEntry{ &s, env_ };
}

void Executor::visitReturnStmt(ReturnStmt& s) {
    LiteralValue val;
    if (s.value) val = evaluate(*s.value);
    throw ReturnException{ std::move(val) };
}

LiteralValue Executor::visitCallExpr(CallExpr& e) {
    auto* varExpr = dynamic_cast<VariableExpr*>(e.callee.get());
    if (!varExpr)
        throw std::runtime_error(
            "[line " + std::to_string(e.paren.line) + "] Call target must be a named function.");

    const std::string& name = varExpr->name.lexeme;

    // ── 사용자 정의 함수 (built-in보다 우선 — 이름 충돌 허용) ──────
    auto fit = functions_.find(name);
    if (fit != functions_.end()) {
        const FunctionEntry& fn     = fit->second;
        const auto&          params = fn.decl->params;

        if (e.arguments.size() != params.size())
            throw std::runtime_error(
                "[line " + std::to_string(e.line) + "] Expected " +
                std::to_string(params.size()) + " arguments but got " +
                std::to_string(e.arguments.size()) + ".");

        std::vector<LiteralValue> argVals;
        argVals.reserve(e.arguments.size());
        for (auto& arg : e.arguments)
            argVals.push_back(evaluate(*arg));

        Environment callEnv(fn.closure);
        for (size_t i = 0; i < params.size(); ++i)
            callEnv.define(params[i].lexeme, argVals[i]);

        ScopeGuard g{env_, std::exchange(env_, &callEnv)};
        LiteralValue result;
        try {
            for (const auto& stmt : fn.decl->body)
                run(*stmt);
        } catch (ReturnException& ret) {
            result = std::move(ret.value);
        }
        return result;
    }

    // ── 배열 생성 내장 함수 ──────────────────────────────────────
    if (name == "Array")
        return callBuiltinArray(e);

    // ── 미정의 ───────────────────────────────────────────────────
    if (env_->contains(name))
        throw std::runtime_error(
            "[line " + std::to_string(e.line) + "] '" + name + "' is not a function.");
    throw std::runtime_error(
        "[line " + std::to_string(e.line) + "] Undefined function '" + name + "'.");
}

void Executor::visitForStmt(ForStmt& s) {
    Environment forScope(env_);
    ScopeGuard g{env_, std::exchange(env_, &forScope)};
    if (s.initializer) run(*s.initializer);
    while (true) {
        if (s.condition && !isTruthy(evaluate(*s.condition))) break;
        run(*s.body);
        if (s.increment) evaluate(*s.increment);
    }
}

// ── 배열 인덱스 ─────────────────────────────────────────────────────
static ArrayPtr requireArray(const LiteralValue& v, int line) {
    auto* p = std::get_if<ArrayPtr>(&v);
    if (!p || !*p)
        throw std::runtime_error("[line " + std::to_string(line) + "] Value is not an array.");
    return *p;
}

static std::size_t requireIndex(const LiteralValue& v, std::size_t size, int line) {
    auto* d = std::get_if<double>(&v);
    if (!d || *d != std::floor(*d))
        throw std::runtime_error("[line " + std::to_string(line) + "] Array index must be an integer.");
    auto i = static_cast<std::ptrdiff_t>(*d);
    if (i < 0 || static_cast<std::size_t>(i) >= size)
        throw std::runtime_error("[line " + std::to_string(line) + "] Array index out of range.");
    return static_cast<std::size_t>(i);
}

LiteralValue Executor::callBuiltinArray(CallExpr& e) {
    if (e.arguments.size() != 1)
        throw std::runtime_error(
            "[line " + std::to_string(e.paren.line) + "] Array() expects exactly 1 argument.");
    LiteralValue sizeVal = evaluate(*e.arguments[0]);
    auto* d = std::get_if<double>(&sizeVal);
    if (!d || *d < 0.0 || *d != std::floor(*d))
        throw std::runtime_error(
            "[line " + std::to_string(e.paren.line) + "] Array size must be a non-negative integer.");
    return std::make_shared<LiteralArray>(static_cast<std::size_t>(*d));
}

LiteralValue Executor::visitIndexExpr(IndexExpr& e) {
    ArrayPtr    arr = requireArray(evaluate(*e.object), e.bracket.line);
    std::size_t i   = requireIndex(evaluate(*e.index), arr->elements.size(), e.bracket.line);
    return arr->elements[i];
}

LiteralValue Executor::visitIndexAssignExpr(IndexAssignExpr& e) {
    IndexExpr&  idx = *e.target;
    ArrayPtr    arr = requireArray(evaluate(*idx.object), idx.bracket.line);
    std::size_t i   = requireIndex(evaluate(*idx.index), arr->elements.size(), idx.bracket.line);
    LiteralValue val = evaluate(*e.value);
    arr->elements[i] = val;
    return val;
}
