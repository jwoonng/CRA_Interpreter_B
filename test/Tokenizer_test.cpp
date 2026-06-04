#include <gtest/gtest.h>
#include "src/assembler/Tokenizer.h"

// Fixture: provides a fresh Tokenizer instance per test
class TokenizerTest : public ::testing::Test {
protected:
    Tokenizer tokenizer;
};

TEST_F(TokenizerTest, VarDeclarationTokens) {
    auto tokens = tokenizer.tokenize("var a = 10;");

    // VAR, IDENTIFIER, EQUAL, NUMBER, SEMICOLON, EOF = 6
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

TEST_F(TokenizerTest, KeywordRecognition) {
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
        ASSERT_EQ(tokens.size(), 2u) << "source: " << src;
        EXPECT_EQ(tokens[0].type,   expected) << "source: " << src;
        EXPECT_EQ(tokens[0].lexeme, src)      << "source: " << src;
    }

    // keyword prefix but treated as identifier
    auto tokens = tokenizer.tokenize("variable");
    ASSERT_EQ(tokens.size(), 2u);
    EXPECT_EQ(tokens[0].type,   TokenType::IDENTIFIER);
    EXPECT_EQ(tokens[0].lexeme, "variable");
}

TEST_F(TokenizerTest, NumberLiteralFormat) {
    struct Case { std::string src; double expected; };
    std::vector<Case> cases = {
        {"42",   42.0},
        {"3.14", 3.14},
        {"0",    0.0},
        {"5.0",  5.0},
    };

    for (auto& [src, expected] : cases) {
        auto tokens = tokenizer.tokenize(src);
        ASSERT_EQ(tokens.size(), 2u) << "source: " << src;
        EXPECT_EQ(tokens[0].type,   TokenType::NUMBER) << "source: " << src;
        EXPECT_EQ(tokens[0].lexeme, src)               << "source: " << src;
        EXPECT_DOUBLE_EQ(std::get<double>(tokens[0].literal), expected) << "source: " << src;
    }
}

TEST_F(TokenizerTest, StringLiteral) {
    // lexeme includes quotes, literal excludes quotes
    {
        auto tokens = tokenizer.tokenize("\"hello\"");
        ASSERT_EQ(tokens.size(), 2u);
        EXPECT_EQ(tokens[0].type,   TokenType::STRING);
        EXPECT_EQ(tokens[0].lexeme, "\"hello\"");
        EXPECT_EQ(std::get<std::string>(tokens[0].literal), "hello");
    }

    // string with spaces
    {
        auto tokens = tokenizer.tokenize("\"hello world\"");
        ASSERT_EQ(tokens.size(), 2u);
        EXPECT_EQ(tokens[0].type,   TokenType::STRING);
        EXPECT_EQ(std::get<std::string>(tokens[0].literal), "hello world");
    }

    // empty string
    {
        auto tokens = tokenizer.tokenize("\"\"");
        ASSERT_EQ(tokens.size(), 2u);
        EXPECT_EQ(tokens[0].type,   TokenType::STRING);
        EXPECT_EQ(std::get<std::string>(tokens[0].literal), "");
    }

    // unterminated string throws
    EXPECT_THROW(tokenizer.tokenize("\"unterminated"), std::runtime_error);
}

TEST_F(TokenizerTest, BoolLiteral) {
    // true/false -> TokenType encodes the value (no literal field needed)
    struct Case { std::string src; TokenType expected; };
    std::vector<Case> keywordCases = {
        {"true",  TokenType::TRUE_KW},
        {"false", TokenType::FALSE_KW},
    };
    for (auto& [src, expected] : keywordCases) {
        auto tokens = tokenizer.tokenize(src);
        ASSERT_EQ(tokens.size(), 2u) << "source: " << src;
        EXPECT_EQ(tokens[0].type,   expected) << "source: " << src;
        EXPECT_EQ(tokens[0].lexeme, src)      << "source: " << src;
    }

    // starts with true/false but treated as identifier
    std::vector<std::string> identCases = {"truex", "falsey"};
    for (auto& src : identCases) {
        auto tokens = tokenizer.tokenize(src);
        ASSERT_EQ(tokens.size(), 2u) << "source: " << src;
        EXPECT_EQ(tokens[0].type, TokenType::IDENTIFIER) << "source: " << src;
    }
}

TEST_F(TokenizerTest, OperatorTokens) {
    struct Case { std::string src; TokenType expected; };

    // single-character operators and delimiters
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

    // two-character operators
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

TEST_F(TokenizerTest, EmptySource) {
    auto tokens = tokenizer.tokenize("");
    ASSERT_EQ(tokens.size(), 1u);
    EXPECT_EQ(tokens[0].type, TokenType::EOF_TOKEN);
}

TEST_F(TokenizerTest, UnknownCharacterThrows) {
    std::vector<std::string> cases = {"@", "#", "$", "~"};
    for (auto& src : cases) {
        EXPECT_THROW(tokenizer.tokenize(src), std::runtime_error) << "source: " << src;
    }
}

TEST_F(TokenizerTest, LineComment) {
    // comment only -> only EOF
    {
        auto tokens = tokenizer.tokenize("// this is a comment");
        ASSERT_EQ(tokens.size(), 1u);
        EXPECT_EQ(tokens[0].type, TokenType::EOF_TOKEN);
    }

    // tokens before comment are tokenized normally
    {
        auto tokens = tokenizer.tokenize("var a = 1; // assign a");
        ASSERT_EQ(tokens.size(), 6u);
        EXPECT_EQ(tokens[0].type, TokenType::VAR);
        EXPECT_EQ(tokens[4].type, TokenType::SEMICOLON);
        EXPECT_EQ(tokens[5].type, TokenType::EOF_TOKEN);
    }
}

TEST_F(TokenizerTest, WhitespaceIgnored) {
    // multiple spaces -- token count unaffected
    {
        auto tokens = tokenizer.tokenize("var   a = 10;");
        ASSERT_EQ(tokens.size(), 6u);
        EXPECT_EQ(tokens[0].type, TokenType::VAR);
        EXPECT_EQ(tokens[1].type, TokenType::IDENTIFIER);
        EXPECT_EQ(tokens[2].type, TokenType::EQUAL);
        EXPECT_EQ(tokens[3].type, TokenType::NUMBER);
    }

    // tabs
    {
        auto tokens = tokenizer.tokenize("var\ta\t=\t10;");
        ASSERT_EQ(tokens.size(), 6u);
        EXPECT_EQ(tokens[0].type, TokenType::VAR);
        EXPECT_EQ(tokens[1].type, TokenType::IDENTIFIER);
    }

    // newlines -- token count same, line number increments
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
