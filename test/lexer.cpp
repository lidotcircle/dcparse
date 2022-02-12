#include <gtest/gtest.h>
#include <vector>
#include <tuple>
#include "lexer/token.h"
#include "lexer/lexer.hpp"
#include "lexer/lexer_rule_regex.hpp"
using namespace std;

class IDToken: public LexerToken {
public:
    string id;
    IDToken(string id, size_t ln, size_t cl, size_t pos, const string& fn): 
        LexerToken(ln, cl, pos, id.size(), fn), id(id) {}
};


TEST(Lexer, basic_test) {
    Lexer<char> lexer("hello.c");
    auto idcreator = [](const vector<char>& id, size_t ln, size_t cl, size_t pos, const string& fn)
    {
        auto ptr = std::make_shared<IDToken>(string(id.begin(), id.end()), ln, cl, pos, fn);
        return std::dynamic_pointer_cast<LexerToken>(ptr);
    };

    lexer.add_rule(std::make_unique<LexerRuleRegex<char>>("[a-zA-Z_][a-zA-Z0-9_]*", idcreator));

    auto t1 = lexer.feed_char('h');
    EXPECT_EQ(t1.size(), 0);
    auto t2 = lexer.feed_char('e');
    EXPECT_EQ(t2.size(), 0);
    auto t3 = lexer.feed_char('l');
    EXPECT_EQ(t3.size(), 0);

    auto t4 = lexer.feed_end();
    EXPECT_EQ(t4.size(), 1);

    t4[0];
}
