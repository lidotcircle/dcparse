#include <gtest/gtest.h>
#include <vector>
#include <tuple>
#include "lexer/token.h"
#include "lexer/lexer.hpp"
#include "lexer/lexer_rule_regex.hpp"
using namespace std;

class TokenID: public LexerToken {
public:
    string id;
    TokenID(string id, LexerToken::TokenInfo info): 
        LexerToken(info), id(id) {}
};

class TokenIF: public LexerToken {
public:
    TokenIF(LexerToken::TokenInfo info): 
        LexerToken(info) {}
};

class TokenBlockComment: public LexerToken {
public:
    string comment;
    TokenBlockComment(string comment, LexerToken::TokenInfo info): 
        LexerToken(info), comment(comment) {}
};


class LexerTest: public ::testing::Test {
protected:
    Lexer<char> lexer;

    void SetUp() override {
        lexer(
            std::make_unique<LexerRuleRegex<char>>(
                "/\\*(!\\*/)\\*/",
                [](auto str, auto info) {
                    return std::make_shared<TokenBlockComment>(
                        string(str.begin(), str.end()), 
                        info);
                })
        );

        lexer.dec_priority_major();

        lexer(
            std::make_unique<LexerRuleRegex<char>>(
                "if",
                [](auto str, auto info) {
                    return std::make_shared<TokenIF>(info);
                })
        );

        lexer.dec_priority_minor();

        lexer(
            std::make_unique<LexerRuleRegex<char>>(
                "[a-zA-Z_][a-zA-Z0-9_]*",
                [](auto str, auto info) {
                return std::make_shared<TokenID>(
                        string(str.begin(), str.end()),
                        info);
            })
        );

        lexer.dec_priority_major();

        // space, tab, newline
        lexer(
            std::make_unique<LexerRuleRegex<char>>(
                "( |\t|\r|\n)+",
                [](auto str, auto info) {
                return nullptr;
            })
        );
    }
};


TEST_F(LexerTest, keywordt1) {
    auto t1 = lexer.feed_char(string("if"));
    EXPECT_EQ(t1.size(), 0);

    auto t2 = lexer.feed_end();
    EXPECT_EQ(t2.size(), 1);

    auto& t = t2[0];
    auto tk2 = std::dynamic_pointer_cast<TokenIF>(t);
    EXPECT_NE(tk2, nullptr);
    EXPECT_EQ(tk2->charid(), CharID<TokenIF>());
}

TEST_F(LexerTest, keywordt2) {
    auto t1 = lexer.feed_char(string("ifl"));
    EXPECT_EQ(t1.size(), 0);

    auto t2 = lexer.feed_end();
    EXPECT_EQ(t2.size(), 1);

    auto& t = t2[0];
    auto tk2 = std::dynamic_pointer_cast<TokenID>(t);
    EXPECT_EQ(tk2->id, "ifl");
}

TEST_F(LexerTest, spacechars) {
    auto t1 = lexer.feed_char(string("if hello world\tif \r if\n iff"));
    EXPECT_EQ(t1.size(), 5);

    auto t2 = lexer.feed_end();
    EXPECT_EQ(t2.size(), 1);

    auto& t = t2[0];
    auto tk2 = std::dynamic_pointer_cast<TokenID>(t);
    EXPECT_EQ(tk2->id, "iff");
}

TEST_F(LexerTest, BlockComment) {
    return;
    auto t1 = lexer.feed_char(string("if /*hello world   fi if ll*/ fi if"));
    EXPECT_EQ(t1.size(), 3);

    auto t2 = lexer.feed_end();
    EXPECT_EQ(t2.size(), 1);
}
