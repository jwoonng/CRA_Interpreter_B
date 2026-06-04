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
    // TODO
}

TEST(TokenizerTest, NumberLiteralFormat) {
    // TODO
}

TEST(TokenizerTest, StringLiteral) {
    // TODO
}

TEST(TokenizerTest, BoolLiteral) {
    // TODO
}

TEST(TokenizerTest, OperatorTokens) {
    // TODO
}

TEST(TokenizerTest, WhitespaceIgnored) {
    // TODO
}
