#pragma once
#include "ITokenizer.h"

class Tokenizer : public ITokenizer {
public:
    std::vector<Token> tokenize(const std::string& source) override;

private:
    class Scanner {
    public:
        explicit Scanner(const std::string& source);
        std::vector<Token> scanAll();

    private:
        bool isAtEnd() const;
        char advance();
        char peek()     const;
        char peekNext() const;
        bool match(char expected);
        void addToken(TokenType type, LiteralValue literal = std::monostate{});

        void scanToken();
        void scanString();
        void scanNumber();
        void scanIdentifier();

        const std::string& source_;
        size_t start_;
        size_t current_;
        int    line_;
        std::vector<Token> tokens_;
    };
};
