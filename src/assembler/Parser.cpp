#include "Parser.h"

// ── 공개 진입점 ────────────────────────────────────────────────
std::vector<std::unique_ptr<Stmt>> Parser::parse(const std::vector<Token>& tokens) {
    tokens_  = tokens;
    current_ = 0;

    std::vector<StmtPtr> stmts;
    while (!isAtEnd())
        stmts.push_back(statement());
    return stmts;
}

// ══════════════════════════════════════════════════════════════
// 문장
// ══════════════════════════════════════════════════════════════

StmtPtr Parser::statement() {
    if (match({TokenType::FUN}))        return functionDeclaration();
    if (match({TokenType::RETURN}))     return returnStatement();
    if (match({TokenType::FOR}))        return forStatement();
    if (match({TokenType::IF}))         return ifStatement();
    if (match({TokenType::PRINT}))      return printStatement();
    if (match({TokenType::VAR}))        return varDeclaration();
    if (match({TokenType::LEFT_BRACE})) return block();
    return expressionStatement();
}

StmtPtr Parser::functionDeclaration() {
    Token name = consume(TokenType::IDENTIFIER, "Expected function name.");
    consume(TokenType::LEFT_PAREN, "Expected '(' after function name.");

    std::vector<Token> params;
    if (!check(TokenType::RIGHT_PAREN)) {
        do {
            params.push_back(consume(TokenType::IDENTIFIER, "Expected parameter name."));
        } while (match({TokenType::COMMA}));
    }
    consume(TokenType::RIGHT_PAREN, "Expected ')' after parameters.");
    consume(TokenType::LEFT_BRACE,  "Expected '{' before function body.");

    std::vector<StmtPtr> body;
    while (!check(TokenType::RIGHT_BRACE) && !isAtEnd())
        body.push_back(statement());
    consume(TokenType::RIGHT_BRACE, "Expected '}' after function body.");

    return std::make_unique<FunctionStmt>(
        std::move(name), std::move(params), std::move(body));
}

StmtPtr Parser::returnStatement() {
    Token keyword = previous();
    ExprPtr value;
    if (!check(TokenType::SEMICOLON))
        value = expression();
    consume(TokenType::SEMICOLON, "Expected ';' after return value.");
    return std::make_unique<ReturnStmt>(std::move(keyword), std::move(value));
}

StmtPtr Parser::forStatement() {
    consume(TokenType::LEFT_PAREN, "Expected '(' after 'for'.");

    StmtPtr initializer;
    if (match({TokenType::SEMICOLON})) {
        // initializer 없음
    } else if (match({TokenType::VAR})) {
        initializer = varDeclaration();
    } else {
        initializer = expressionStatement();
    }

    ExprPtr condition;
    if (!check(TokenType::SEMICOLON))
        condition = expression();
    consume(TokenType::SEMICOLON, "Expected ';' after for condition.");

    ExprPtr increment;
    if (!check(TokenType::RIGHT_PAREN))
        increment = expression();
    consume(TokenType::RIGHT_PAREN, "Expected ')' after for clauses.");

    StmtPtr body = statement();

    return std::make_unique<ForStmt>(
        std::move(initializer),
        std::move(condition),
        std::move(increment),
        std::move(body)
    );
}

StmtPtr Parser::ifStatement() {
    consume(TokenType::LEFT_PAREN, "Expected '(' after 'if'.");
    ExprPtr condition = expression();
    consume(TokenType::RIGHT_PAREN, "Expected ')' after if condition.");

    StmtPtr thenBranch = statement();
    StmtPtr elseBranch;
    if (match({TokenType::ELSE}))
        elseBranch = statement();

    return std::make_unique<IfStmt>(
        std::move(condition),
        std::move(thenBranch),
        std::move(elseBranch)
    );
}

StmtPtr Parser::printStatement() {
    int line    = previous().line;
    ExprPtr val = expression();
    consume(TokenType::SEMICOLON, "Expected ';' after value.");
    return std::make_unique<PrintStmt>(std::move(val), line);
}

StmtPtr Parser::varDeclaration() {
    Token name = consume(TokenType::IDENTIFIER, "Expected variable name.");

    ExprPtr initializer;
    if (match({TokenType::EQUAL}))
        initializer = expression();

    consume(TokenType::SEMICOLON, "Expected ';' after variable declaration.");
    return std::make_unique<VarDeclareStmt>(std::move(name), std::move(initializer));
}

StmtPtr Parser::block() {
    std::vector<StmtPtr> stmts;
    while (!check(TokenType::RIGHT_BRACE) && !isAtEnd())
        stmts.push_back(statement());
    consume(TokenType::RIGHT_BRACE, "Expected '}' after block.");
    return std::make_unique<BlockStmt>(std::move(stmts));
}

StmtPtr Parser::expressionStatement() {
    ExprPtr expr = expression();
    consume(TokenType::SEMICOLON, "Expected ';' after expression.");
    return std::make_unique<ExpressionStmt>(std::move(expr));
}

// ══════════════════════════════════════════════════════════════
// 표현식 (우선순위 낮은 순)
// ══════════════════════════════════════════════════════════════

ExprPtr Parser::expression() {
    return assignment();
}

ExprPtr Parser::assignment() {
    ExprPtr expr = logicalOr();

    if (match({TokenType::EQUAL})) {
        Token   eq  = previous();
        ExprPtr rhs = assignment();

        if (auto* var = dynamic_cast<VariableExpr*>(expr.get()))
            return std::make_unique<AssignExpr>(var->name, std::move(rhs));

        throw error(eq, "Invalid assignment target.");
    }

    return expr;
}

ExprPtr Parser::logicalOr() {
    return parseBinaryLeft<LogicalExpr>([this] { return logicalAnd(); }, {TokenType::OR});
}

ExprPtr Parser::logicalAnd() {
    return parseBinaryLeft<LogicalExpr>([this] { return equality(); }, {TokenType::AND});
}

ExprPtr Parser::equality() {
    return parseBinaryLeft<BinaryExpr>([this] { return comparison(); },
                                       {TokenType::BANG_EQUAL, TokenType::EQUAL_EQUAL});
}

ExprPtr Parser::comparison() {
    return parseBinaryLeft<BinaryExpr>([this] { return term(); },
                                       {TokenType::GREATER, TokenType::GREATER_EQUAL,
                                        TokenType::LESS,    TokenType::LESS_EQUAL});
}

ExprPtr Parser::term() {
    return parseBinaryLeft<BinaryExpr>([this] { return factor(); },
                                       {TokenType::PLUS, TokenType::MINUS});
}

ExprPtr Parser::factor() {
    return parseBinaryLeft<BinaryExpr>([this] { return unary(); },
                                       {TokenType::STAR, TokenType::SLASH});
}

ExprPtr Parser::unary() {
    if (match({TokenType::BANG, TokenType::MINUS})) {
        Token   op  = previous();
        ExprPtr rhs = unary();
        return std::make_unique<UnaryExpr>(std::move(op), std::move(rhs));
    }
    return call();
}

ExprPtr Parser::call() {
    ExprPtr expr = primary();
    while (match({TokenType::LEFT_PAREN}))
        expr = finishCall(std::move(expr));
    return expr;
}

ExprPtr Parser::finishCall(ExprPtr callee) {
    std::vector<ExprPtr> args;
    if (!check(TokenType::RIGHT_PAREN)) {
        do {
            args.push_back(expression());
        } while (match({TokenType::COMMA}));
    }
    Token paren = consume(TokenType::RIGHT_PAREN, "Expected ')' after arguments.");
    return std::make_unique<CallExpr>(std::move(callee), std::move(paren), std::move(args));
}

ExprPtr Parser::primary() {
    if (match({TokenType::FALSE_KW}))
        return std::make_unique<LiteralExpr>(LiteralValue{false}, previous().line);

    if (match({TokenType::TRUE_KW}))
        return std::make_unique<LiteralExpr>(LiteralValue{true}, previous().line);

    if (match({TokenType::NUMBER, TokenType::STRING})) {
        const auto& prev = previous();
        return std::make_unique<LiteralExpr>(prev.literal, prev.line);
    }

    if (match({TokenType::IDENTIFIER}))
        return std::make_unique<VariableExpr>(previous());

    if (match({TokenType::LEFT_PAREN})) {
        ExprPtr expr = expression();
        consume(TokenType::RIGHT_PAREN, "Expected ')' after expression.");
        return std::make_unique<GroupingExpr>(std::move(expr));
    }

    throw error(peek(), "Expected expression.");
}

// ══════════════════════════════════════════════════════════════
// 유틸리티
// ══════════════════════════════════════════════════════════════

const Token& Parser::peek()     const { return tokens_[current_]; }
const Token& Parser::previous() const { return tokens_[current_ - 1]; }
bool         Parser::isAtEnd()  const { return peek().type == TokenType::EOF_TOKEN; }

void Parser::advance() {
    if (!isAtEnd()) current_++;
}

bool Parser::check(TokenType type) const {
    return !isAtEnd() && peek().type == type;
}

bool Parser::match(std::initializer_list<TokenType> types) {
    for (auto t : types) {
        if (check(t)) { advance(); return true; }
    }
    return false;
}

Token Parser::consume(TokenType type, const std::string& message) {
    if (check(type)) { advance(); return previous(); }
    throw error(peek(), message);
}

std::runtime_error Parser::error(const Token& token, const std::string& message) const {
    return std::runtime_error("[line " + std::to_string(token.line) + "] " + message);
}
