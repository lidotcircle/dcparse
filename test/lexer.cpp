#include "lexer/lexer.hpp"
#include "lexer/lexer_rule_cstring_literal.hpp"
#include "lexer/lexer_rule_regex.hpp"
#include "lexer/simple_lexer.hpp"
#include "lexer/token.h"
#include <gtest/gtest.h>
#include <tuple>
#include <vector>
using namespace std;

class TokenID : public LexerToken
{
  public:
    string id;
    TokenID(string id, TextRange info) : LexerToken(info), id(id)
    {}
};

class TokenIF : public LexerToken
{
  public:
    TokenIF(TextRange info) : LexerToken(info)
    {}
};

class TokenBlockComment : public LexerToken
{
  public:
    string comment;
    TokenBlockComment(string comment, TextRange info) : LexerToken(info), comment(comment)
    {}
};

class TokenStringLiteral : public LexerToken
{
  public:
    string literal;
    TokenStringLiteral(string literal, TextRange info) : LexerToken(info), literal(literal)
    {}
};


class LexerTest : public ::testing::Test
{
  protected:
    Lexer<char> lexer;

    void SetUp() override
    {
        lexer(
            std::make_unique<LexerRuleRegex<char>>("/\\*(!.*\\*/.*)\\*/", [](auto str, auto info) {
                return std::make_shared<TokenBlockComment>(string(str.begin(), str.end()), info);
            }));

        lexer.dec_priority_major();

        lexer(std::make_unique<LexerRuleCStringLiteral<char>>([](auto str, auto info) {
            return std::make_shared<TokenStringLiteral>(string(str.begin(), str.end()), info);
        }));

        lexer.dec_priority_major();

        lexer(std::make_unique<LexerRuleRegex<char>>(
            "if", [](auto str, auto info) { return std::make_shared<TokenIF>(info); }));

        lexer.dec_priority_minor();

        lexer(std::make_unique<LexerRuleRegex<char>>(
            "[a-zA-Z_][a-zA-Z0-9_]*", [](auto str, auto info) {
                return std::make_shared<TokenID>(string(str.begin(), str.end()), info);
            }));

        lexer.dec_priority_major();

        // space, tab, newline
        lexer(std::make_unique<LexerRuleRegex<char>>("( |\t|\r|\n)+",
                                                     [](auto str, auto info) { return nullptr; }));
    }
};


TEST_F(LexerTest, keywordt1)
{
    auto t1 = lexer.feed_char(string("if"));
    EXPECT_EQ(t1.size(), 0);

    auto t2 = lexer.feed_end();
    EXPECT_EQ(t2.size(), 1);

    auto& t = t2[0];
    auto tk2 = std::dynamic_pointer_cast<TokenIF>(t);
    EXPECT_NE(tk2, nullptr);
    EXPECT_EQ(tk2->charid(), CharID<TokenIF>());
}

TEST_F(LexerTest, keywordt2)
{
    auto t1 = lexer.feed_char(string("ifl"));
    EXPECT_EQ(t1.size(), 0);

    auto t2 = lexer.feed_end();
    EXPECT_EQ(t2.size(), 1);

    auto& t = t2[0];
    auto tk2 = std::dynamic_pointer_cast<TokenID>(t);
    EXPECT_EQ(tk2->id, "ifl");
}

TEST_F(LexerTest, spacechars)
{
    auto t1 = lexer.feed_char(string("if hello world\tif \r if\n iff"));
    EXPECT_EQ(t1.size(), 5);

    auto t2 = lexer.feed_end();
    EXPECT_EQ(t2.size(), 1);

    auto& t = t2[0];
    auto tk2 = std::dynamic_pointer_cast<TokenID>(t);
    EXPECT_EQ(tk2->id, "iff");
}

TEST_F(LexerTest, BlockComment)
{
    auto t1 = lexer.feed_char(string("if /*hello world   fi if ll*/ fi if"));
    EXPECT_EQ(t1.size(), 3);

    auto t2 = lexer.feed_end();
    EXPECT_EQ(t2.size(), 1);
}

TEST_F(LexerTest, StringLiteral)
{
    auto t1 = lexer.feed_char(string("if /*hello world   fi if ll*/ fi if \"hello \\\"world\""));
    EXPECT_EQ(t1.size(), 4);

    auto t2 = lexer.feed_end();
    EXPECT_EQ(t2.size(), 1);
    EXPECT_EQ(t2[0]->length().value(), 15);
    auto stoken = std::dynamic_pointer_cast<TokenStringLiteral>(t2[0]);
    EXPECT_NE(stoken, nullptr);
    EXPECT_EQ(stoken->literal, "hello \"world");
}

TEST_F(LexerTest, SimpleLexer)
{
    string str = "if /*hello world   fi if ll*/ fi if \"hello \\\"world\"";
    SimpleLexer<char> slexer(std::make_unique<Lexer<char>>(std::move(lexer)),
                             vector<char>(str.begin(), str.end()));
    EXPECT_FALSE(slexer.end());

    auto t1 = slexer.next();
    EXPECT_FALSE(slexer.end());

    auto t2 = slexer.next();
    EXPECT_FALSE(slexer.end());

    auto t3 = slexer.next();
    EXPECT_FALSE(slexer.end());

    auto t4 = slexer.next();
    EXPECT_FALSE(slexer.end());

    auto t5 = slexer.next();
    EXPECT_TRUE(slexer.end());

    slexer.back();
    EXPECT_FALSE(slexer.end());

    t5 = slexer.next();
    EXPECT_TRUE(slexer.end());

    EXPECT_THROW(slexer.next(), LexerError);
}
