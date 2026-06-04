#include <gtest/gtest.h>
#include "src/assembler/Tokenizer.h"

// ── 개발자 1 담당 영역 ──────────────────────────────
// 테스트 작성 순서: Red → Green → Refactor

TEST(TokenizerTest, VarDeclarationTokens) {
    Tokenizer tokenizer;
    auto tokens = tokenizer.tokenize("var a = 10;");

    // VAR, IDENTIFIER, EQUAL, NUMBER, SEMICOLON, EOF = 6개
    ASSERT_EQ(tokens.size(), 6u);

    EXPECT_EQ(tokens[0].type,   TokenType::VAR);
    EXPECT_EQ(tokens[0].lexeme, "var");

    EXPECT_EQ(tokens[1].type,   TokenType::IDENTIFIER);
    EXPECT_EQ(tokens[1].lexeme, "a");

    EXPECT_EQ(tokens[2].type,   TokenType::EQUAL);
    EXPECT_EQ(tokens[2].lexeme, "=");

    EXPECT_EQ(tokens[3].type,   TokenType::NUMBER);
    EXPECT_EQ(tokens[3].lexeme, "10");
    EXPECT_DOUBLE_EQ(std::get<double>(tokens[3].literal), 10.0);

    EXPECT_EQ(tokens[4].type,   TokenType::SEMICOLON);
    EXPECT_EQ(tokens[4].lexeme, ";");

    EXPECT_EQ(tokens[5].type,   TokenType::EOF_TOKEN);
}

TEST(TokenizerTest, KeywordRecognition) {
    Tokenizer tokenizer;

    // 모든 키워드 → 올바른 TokenType + lexeme 확인
    struct Case { std::string src; TokenType expected; };
    std::vector<Case> cases = {
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

    for (auto& [src, expected] : cases) {
        auto tokens = tokenizer.tokenize(src);
        ASSERT_EQ(tokens.size(), 2u) << "source: " << src; // keyword + EOF
        EXPECT_EQ(tokens[0].type,   expected) << "source: " << src;
        EXPECT_EQ(tokens[0].lexeme, src)      << "source: " << src;
    }

    // 키워드처럼 시작하지만 식별자인 경우
    auto tokens = tokenizer.tokenize("variable");
    ASSERT_EQ(tokens.size(), 2u);
    EXPECT_EQ(tokens[0].type,   TokenType::IDENTIFIER);
    EXPECT_EQ(tokens[0].lexeme, "variable");
}

TEST(TokenizerTest, NumberLiteralFormat) {
    Tokenizer tokenizer;

    // 정수 → NUMBER 토큰, literal은 double
    {
        auto tokens = tokenizer.tokenize("42");
        ASSERT_EQ(tokens.size(), 2u);
        EXPECT_EQ(tokens[0].type,   TokenType::NUMBER);
        EXPECT_EQ(tokens[0].lexeme, "42");
        EXPECT_DOUBLE_EQ(std::get<double>(tokens[0].literal), 42.0);
    }

    // 소수점 숫자
    {
        auto tokens = tokenizer.tokenize("3.14");
        ASSERT_EQ(tokens.size(), 2u);
        EXPECT_EQ(tokens[0].type,   TokenType::NUMBER);
        EXPECT_EQ(tokens[0].lexeme, "3.14");
        EXPECT_DOUBLE_EQ(std::get<double>(tokens[0].literal), 3.14);
    }

    // 0
    {
        auto tokens = tokenizer.tokenize("0");
        ASSERT_EQ(tokens.size(), 2u);
        EXPECT_EQ(tokens[0].type,   TokenType::NUMBER);
        EXPECT_EQ(tokens[0].lexeme, "0");
        EXPECT_DOUBLE_EQ(std::get<double>(tokens[0].literal), 0.0);
    }

    // 5.0 → lexeme "5.0", literal 5.0
    {
        auto tokens = tokenizer.tokenize("5.0");
        ASSERT_EQ(tokens.size(), 2u);
        EXPECT_EQ(tokens[0].type,   TokenType::NUMBER);
        EXPECT_EQ(tokens[0].lexeme, "5.0");
        EXPECT_DOUBLE_EQ(std::get<double>(tokens[0].literal), 5.0);
    }
}

TEST(TokenizerTest, StringLiteral) {
    Tokenizer tokenizer;

    // 기본 문자열 — lexeme은 따옴표 포함, literal은 따옴표 제외
    {
        auto tokens = tokenizer.tokenize("\"hello\"");
        ASSERT_EQ(tokens.size(), 2u);
        EXPECT_EQ(tokens[0].type,   TokenType::STRING);
        EXPECT_EQ(tokens[0].lexeme, "\"hello\"");
        EXPECT_EQ(std::get<std::string>(tokens[0].literal), "hello");
    }

    // 공백 포함 문자열
    {
        auto tokens = tokenizer.tokenize("\"hello world\"");
        ASSERT_EQ(tokens.size(), 2u);
        EXPECT_EQ(tokens[0].type,   TokenType::STRING);
        EXPECT_EQ(std::get<std::string>(tokens[0].literal), "hello world");
    }

    // 빈 문자열
    {
        auto tokens = tokenizer.tokenize("\"\"");
        ASSERT_EQ(tokens.size(), 2u);
        EXPECT_EQ(tokens[0].type,   TokenType::STRING);
        EXPECT_EQ(std::get<std::string>(tokens[0].literal), "");
    }

    // 닫는 따옴표 없으면 예외
    {
        EXPECT_THROW(tokenizer.tokenize("\"unterminated"), std::runtime_error);
    }
}

TEST(TokenizerTest, BoolLiteral) {
    Tokenizer tokenizer;

    // true → TRUE_KW (값은 TokenType 자체로 표현, literal 불필요)
    {
        auto tokens = tokenizer.tokenize("true");
        ASSERT_EQ(tokens.size(), 2u);
        EXPECT_EQ(tokens[0].type,   TokenType::TRUE_KW);
        EXPECT_EQ(tokens[0].lexeme, "true");
    }

    // false → FALSE_KW
    {
        auto tokens = tokenizer.tokenize("false");
        ASSERT_EQ(tokens.size(), 2u);
        EXPECT_EQ(tokens[0].type,   TokenType::FALSE_KW);
        EXPECT_EQ(tokens[0].lexeme, "false");
    }

    // true/false로 시작하지만 식별자인 경우
    {
        auto tokens = tokenizer.tokenize("truex");
        ASSERT_EQ(tokens.size(), 2u);
        EXPECT_EQ(tokens[0].type, TokenType::IDENTIFIER);
    }
    {
        auto tokens = tokenizer.tokenize("falsey");
        ASSERT_EQ(tokens.size(), 2u);
        EXPECT_EQ(tokens[0].type, TokenType::IDENTIFIER);
    }
}

TEST(TokenizerTest, OperatorTokens) {
    Tokenizer tokenizer;

    struct Case { std::string src; TokenType expected; };

    // 단일 문자 연산자 / 구분자
    std::vector<Case> singleCases = {
        {"+", TokenType::PLUS},    {"-", TokenType::MINUS},
        {"*", TokenType::STAR},    {"/", TokenType::SLASH},
        {"!", TokenType::BANG},    {"=", TokenType::EQUAL},
        {">", TokenType::GREATER}, {"<", TokenType::LESS},
        {"(", TokenType::LEFT_PAREN},  {")", TokenType::RIGHT_PAREN},
        {"{", TokenType::LEFT_BRACE},  {"}", TokenType::RIGHT_BRACE},
        {";", TokenType::SEMICOLON},
    };

    for (auto& [src, expected] : singleCases) {
        auto tokens = tokenizer.tokenize(src);
        ASSERT_EQ(tokens.size(), 2u) << "source: " << src;
        EXPECT_EQ(tokens[0].type,   expected) << "source: " << src;
        EXPECT_EQ(tokens[0].lexeme, src)      << "source: " << src;
    }

    // 두 문자 연산자 — 앞 문자만 보면 다른 토큰이 되는 케이스
    std::vector<Case> doubleCases = {
        {"==", TokenType::EQUAL_EQUAL},
        {"!=", TokenType::BANG_EQUAL},
        {">=", TokenType::GREATER_EQUAL},
        {"<=", TokenType::LESS_EQUAL},
    };

    for (auto& [src, expected] : doubleCases) {
        auto tokens = tokenizer.tokenize(src);
        ASSERT_EQ(tokens.size(), 2u) << "source: " << src;
        EXPECT_EQ(tokens[0].type,   expected) << "source: " << src;
        EXPECT_EQ(tokens[0].lexeme, src)      << "source: " << src;
    }
}

TEST(TokenizerTest, WhitespaceIgnored) {
    Tokenizer tokenizer;

    // 스페이스 여러 개 — 토큰 수에 영향 없음
    {
        auto tokens = tokenizer.tokenize("var   a = 10;");
        ASSERT_EQ(tokens.size(), 6u);
        EXPECT_EQ(tokens[0].type, TokenType::VAR);
        EXPECT_EQ(tokens[1].type, TokenType::IDENTIFIER);
        EXPECT_EQ(tokens[2].type, TokenType::EQUAL);
        EXPECT_EQ(tokens[3].type, TokenType::NUMBER);
    }

    // 탭 포함
    {
        auto tokens = tokenizer.tokenize("var\ta\t=\t10;");
        ASSERT_EQ(tokens.size(), 6u);
        EXPECT_EQ(tokens[0].type, TokenType::VAR);
        EXPECT_EQ(tokens[1].type, TokenType::IDENTIFIER);
    }

    // 개행 포함 — 토큰 수는 동일하고 line 번호가 증가
    {
        auto tokens = tokenizer.tokenize("var\na\n=\n10;");
        ASSERT_EQ(tokens.size(), 6u);
        EXPECT_EQ(tokens[0].type, TokenType::VAR);
        EXPECT_EQ(tokens[0].line, 1);
        EXPECT_EQ(tokens[1].type, TokenType::IDENTIFIER);
        EXPECT_EQ(tokens[1].line, 2);
        EXPECT_EQ(tokens[2].type, TokenType::EQUAL);
        EXPECT_EQ(tokens[2].line, 3);
        EXPECT_EQ(tokens[3].type, TokenType::NUMBER);
        EXPECT_EQ(tokens[3].line, 4);
    }
}
