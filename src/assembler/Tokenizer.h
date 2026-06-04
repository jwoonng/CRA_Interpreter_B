#pragma once
#include "ITokenizer.h"
#include <unordered_map>

class Tokenizer : public ITokenizer {
public:
    std::vector<Token> tokenize(const std::string& source) override;

private:
    class Scanner {
    public:
        explicit Scanner(const std::string& source);
        std::vector<Token> scanAll();

    private:
        // scan methods
        void scanToken();
        void scanString();
        void scanNumber();
        void scanIdentifier();

        // navigation helpers
        bool isAtEnd()  const;
        char advance();
        char peek()     const;
        char peekNext() const;
        bool match(char expected);

        // token creation
        void        addToken(TokenType type, LiteralValue literal = std::monostate{});
        std::string currentLexeme() const;

        // keyword table
        static const std::unordered_map<std::string, TokenType> KEYWORDS;

        // state
        const std::string& source_;
        size_t start_;
        size_t current_;
        int    line_;
        std::vector<Token> tokens_;
    };
};
