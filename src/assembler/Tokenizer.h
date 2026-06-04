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
        // 스캔 메서드 — 토큰 분류 및 수집
        void scanToken();
        void scanString();
        void scanNumber();
        void scanIdentifier();

        // 문자 탐색 헬퍼 — 소스 순회
        bool isAtEnd()  const;
        char advance();
        char peek()     const;
        char peekNext() const;
        bool match(char expected);

        // 토큰 생성
        void addToken(TokenType type, LiteralValue literal = std::monostate{});

        // 키워드 테이블
        static const std::unordered_map<std::string, TokenType> KEYWORDS;

        // 상태
        const std::string& source_;
        size_t start_;
        size_t current_;
        int    line_;
        std::vector<Token> tokens_;
    };
};
