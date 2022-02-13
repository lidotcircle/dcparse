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


TEST(Lexer, basic_test) {
    Lexer<char> lexer("hello.c");

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

    auto t1 = lexer.feed_char('h');
    EXPECT_EQ(t1.size(), 0);
    auto t2 = lexer.feed_char('e');
    EXPECT_EQ(t2.size(), 0);
    auto t3 = lexer.feed_char('l');
    EXPECT_EQ(t3.size(), 0);

    auto t4 = lexer.feed_end();
    EXPECT_EQ(t4.size(), 1);

    auto& t = t4[0];
    auto tk2 = std::dynamic_pointer_cast<TokenID>(t);
    EXPECT_EQ(tk2->id, "hel");
}
