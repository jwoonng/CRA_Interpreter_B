#include "Tokenizer.h"
#include <cctype>
#include <stdexcept>
#include <unordered_map>

namespace {

const std::unordered_map<std::string, TokenType> KEYWORDS = {
    {"var",   TokenType::VAR},
    {"if",    TokenType::IF},
    {"else",  TokenType::ELSE},
    {"for",   TokenType::FOR},
    {"true",  TokenType::TRUE_KW},
    {"false", TokenType::FALSE_KW},
    {"and",   TokenType::AND},
    {"or",    TokenType::OR},
    {"print", TokenType::PRINT},
};

} // namespace

// ── Scanner 구현 ─────────────────────────────────────────

Tokenizer::Scanner::Scanner(const std::string& source)
    : source_(source), start_(0), current_(0), line_(1) {}

std::vector<Token> Tokenizer::Scanner::scanAll() {
    while (!isAtEnd()) {
        start_ = current_;
        scanToken();
    }
    tokens_.push_back({TokenType::EOF_TOKEN, "", std::monostate{}, line_});
    return tokens_;
}

bool Tokenizer::Scanner::isAtEnd() const {
    return current_ >= source_.size();
}

char Tokenizer::Scanner::advance() {
    return source_[current_++];
}

char Tokenizer::Scanner::peek() const {
    return isAtEnd() ? '\0' : source_[current_];
}

char Tokenizer::Scanner::peekNext() const {
    return (current_ + 1 >= source_.size()) ? '\0' : source_[current_ + 1];
}

bool Tokenizer::Scanner::match(char expected) {
    if (isAtEnd() || source_[current_] != expected) return false;
    current_++;
    return true;
}

void Tokenizer::Scanner::addToken(TokenType type, LiteralValue literal) {
    tokens_.push_back({type, source_.substr(start_, current_ - start_), literal, line_});
}

void Tokenizer::Scanner::scanToken() {
    char c = advance();
    switch (c) {
        case '(': addToken(TokenType::LEFT_PAREN);  break;
        case ')': addToken(TokenType::RIGHT_PAREN); break;
        case '{': addToken(TokenType::LEFT_BRACE);  break;
        case '}': addToken(TokenType::RIGHT_BRACE); break;
        case ';': addToken(TokenType::SEMICOLON);   break;
        case '+': addToken(TokenType::PLUS);        break;
        case '-': addToken(TokenType::MINUS);       break;
        case '*': addToken(TokenType::STAR);        break;
        case '/': addToken(TokenType::SLASH);       break;
        case '!': addToken(match('=') ? TokenType::BANG_EQUAL    : TokenType::BANG);           break;
        case '=': addToken(match('=') ? TokenType::EQUAL_EQUAL   : TokenType::EQUAL);          break;
        case '<': addToken(match('=') ? TokenType::LESS_EQUAL    : TokenType::LESS);           break;
        case '>': addToken(match('=') ? TokenType::GREATER_EQUAL : TokenType::GREATER);        break;
        case ' ': case '\r': case '\t': break;
        case '\n': line_++; break;
        case '"': scanString(); break;
        default:
            if (std::isdigit(static_cast<unsigned char>(c)))                          { scanNumber();     break; }
            if (std::isalpha(static_cast<unsigned char>(c)) || c == '_')              { scanIdentifier(); break; }
            throw std::runtime_error(
                "[line " + std::to_string(line_) + "] Unexpected character: " + c);
    }
}

void Tokenizer::Scanner::scanString() {
    while (peek() != '"' && !isAtEnd()) {
        if (peek() == '\n') line_++;
        advance();
    }
    if (isAtEnd())
        throw std::runtime_error(
            "[line " + std::to_string(line_) + "] Unterminated string");
    advance(); // 닫는 "
    std::string value = source_.substr(start_ + 1, current_ - start_ - 2);
    addToken(TokenType::STRING, value);
}

void Tokenizer::Scanner::scanNumber() {
    while (std::isdigit(static_cast<unsigned char>(peek()))) advance();
    if (peek() == '.' && std::isdigit(static_cast<unsigned char>(peekNext()))) {
        advance();
        while (std::isdigit(static_cast<unsigned char>(peek()))) advance();
    }
    double value = std::stod(source_.substr(start_, current_ - start_));
    addToken(TokenType::NUMBER, value);
}

void Tokenizer::Scanner::scanIdentifier() {
    while (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_') advance();
    std::string text = source_.substr(start_, current_ - start_);
    auto it = KEYWORDS.find(text);
    addToken(it != KEYWORDS.end() ? it->second : TokenType::IDENTIFIER);
}

// ── Tokenizer 진입점 ──────────────────────────────────────

std::vector<Token> Tokenizer::tokenize(const std::string& source) {
    return Scanner(source).scanAll();
}
