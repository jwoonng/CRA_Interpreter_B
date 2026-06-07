#include "Tokenizer.h"
#include <cctype>
#include <stdexcept>
#include <unordered_map>

// -- static member definition --

const std::unordered_map<std::string, TokenType> Tokenizer::Scanner::KEYWORDS = {
    {"var",    TokenType::VAR},
    {"if",     TokenType::IF},
    {"else",   TokenType::ELSE},
    {"for",    TokenType::FOR},
    {"true",   TokenType::TRUE_KW},
    {"false",  TokenType::FALSE_KW},
    {"and",    TokenType::AND},
    {"or",     TokenType::OR},
    {"print",  TokenType::PRINT},
    {"Func",   TokenType::FUN},
    {"return", TokenType::RETURN},
};

// -- constructor / entry point --

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

// -- scan methods --

void Tokenizer::Scanner::scanToken() {
    char c = advance();
    switch (c) {
        case '(': addToken(TokenType::LEFT_PAREN);  break;
        case ')': addToken(TokenType::RIGHT_PAREN); break;
        case '{': addToken(TokenType::LEFT_BRACE);  break;
        case '}': addToken(TokenType::RIGHT_BRACE); break;
        case ';': addToken(TokenType::SEMICOLON);   break;
        case ',': addToken(TokenType::COMMA);       break;
        case '[': addToken(TokenType::LEFT_BRACKET);  break;
        case ']': addToken(TokenType::RIGHT_BRACKET); break;
        case '+':
            if (match('+')) throw std::runtime_error(
                "[line " + std::to_string(line_) + "] Unsupported operator '++'. Use 'x = x + 1'.");
            if (match('=')) throw std::runtime_error(
                "[line " + std::to_string(line_) + "] Unsupported operator '+='. Use 'x = x + value'.");
            addToken(TokenType::PLUS);
            break;
        case '-':
            if (match('-')) throw std::runtime_error(
                "[line " + std::to_string(line_) + "] Unsupported operator '--'. Use 'x = x - 1'.");
            if (match('=')) throw std::runtime_error(
                "[line " + std::to_string(line_) + "] Unsupported operator '-='. Use 'x = x - value'.");
            addToken(TokenType::MINUS);
            break;
        case '*':
            if (match('=')) throw std::runtime_error(
                "[line " + std::to_string(line_) + "] Unsupported operator '*='. Use 'x = x * value'.");
            addToken(TokenType::STAR);
            break;
        case '%': addToken(TokenType::PERCENT);     break;
        case '/':
            if (match('/')) skipLineComment();
            else if (match('=')) throw std::runtime_error(
                "[line " + std::to_string(line_) + "] Unsupported operator '/='. Use 'x = x / value'.");
            else            addToken(TokenType::SLASH);
            break;
        case '!': addToken(match('=') ? TokenType::BANG_EQUAL    : TokenType::BANG);    break;
        case '=': addToken(match('=') ? TokenType::EQUAL_EQUAL   : TokenType::EQUAL);   break;
        case '<': addToken(match('=') ? TokenType::LESS_EQUAL    : TokenType::LESS);    break;
        case '>': addToken(match('=') ? TokenType::GREATER_EQUAL : TokenType::GREATER); break;
        case ' ': case '\r': case '\t': break;
        case '\n': line_++; break;
        case '"': scanString(); break;
        default:
            if (std::isdigit(static_cast<unsigned char>(c)))
                scanNumber();
            else if (std::isalpha(static_cast<unsigned char>(c)) || c == '_')
                scanIdentifier();
            else
                throw std::runtime_error(
                    "[line " + std::to_string(line_) + "] Unexpected character: " + c);
    }
}

void Tokenizer::Scanner::skipLineComment() {
    while (peek() != '\n' && !isAtEnd()) advance();
}

void Tokenizer::Scanner::scanString() {
    while (peek() != '"' && !isAtEnd()) {
        if (peek() == '\n') line_++;
        advance();
    }
    if (isAtEnd())
        throw std::runtime_error(
            "[line " + std::to_string(line_) + "] Unterminated string");
    advance(); // closing "
    std::string value = source_.substr(start_ + 1, current_ - start_ - 2);
    addToken(TokenType::STRING, value);
}

void Tokenizer::Scanner::scanNumber() {
    while (std::isdigit(static_cast<unsigned char>(peek()))) advance();
    if (peek() == '.' && std::isdigit(static_cast<unsigned char>(peekNext()))) {
        advance();
        while (std::isdigit(static_cast<unsigned char>(peek()))) advance();
    }
    addToken(TokenType::NUMBER, std::stod(currentLexeme()));
}

void Tokenizer::Scanner::scanIdentifier() {
    while (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_') advance();
    auto it = KEYWORDS.find(currentLexeme());
    addToken(it != KEYWORDS.end() ? it->second : TokenType::IDENTIFIER);
}

// -- navigation helpers --

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

// -- token creation --

std::string Tokenizer::Scanner::currentLexeme() const {
    return source_.substr(start_, current_ - start_);
}

void Tokenizer::Scanner::addToken(TokenType type, LiteralValue literal) {
    tokens_.push_back({type, currentLexeme(), literal, line_});
}

// -- Tokenizer entry point --

std::vector<Token> Tokenizer::tokenize(const std::string& source) {
    return Scanner(source).scanAll();
}
