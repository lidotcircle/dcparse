#include <gtest/gtest.h>
#include "c_token.h"
#include <iostream>
#include <vector>
#include <string>
using namespace std;


TEST(rulesInit, CLexerBasic) {
    EXPECT_NO_THROW(
        cparser::CLexerUTF8 lexer;
    );
}

string tokens2str(vector<shared_ptr<LexerToken>> tokens) {
    string result;
    for (auto token : tokens)
        result += string(token->charname()) + "[" + to_string(token->position()) + "] ";
    return result;
}

TEST(should_accept, CLexerBasic) {
    cparser::CLexerUTF8 lexer;

    vector<pair<string,size_t>> test_cases = {
        { "int", 1 },
        { "_a _b c d e f", 6 },
        { "\"int\"", 1 },
        { "\"\"", 1 },
        { "\"\\\"\"", 1 },
        { "\"a\\a\\b\\c\\d\\\"abcd\"", 1 },
        { "/* asdfzxcv  */", 0 },
        { "/* asdfzxcv  */ */", 2 },
        { "/**", 3 },
        { "// asdfzxcv", 0 },
    };

    for (auto ts: test_cases) {
        auto t = ts.first;
        auto n = ts.second;

        vector<shared_ptr<LexerToken>> tokens;
        for (auto c: t) {
            for (auto x: lexer.feed(c))
                tokens.push_back(x);
        }
        for (auto x: lexer.end())
            tokens.push_back(x);

        EXPECT_EQ(n, tokens.size()) << t << " " << n << endl
            << tokens2str(tokens);
        assert(n == tokens.size());

        lexer.reset();
    }
}

TEST(shoud_reject, CLexerBasic) {
    cparser::CLexerUTF8 lexer;

    vector<string> test_cases = {
        "$",
    };

    for (auto ts: test_cases) {

        EXPECT_ANY_THROW(
            vector<shared_ptr<LexerToken>> tokens;
            for (auto c: ts) {
                for (auto x: lexer.feed(c))
                    tokens.push_back(x);
            }
            for (auto x: lexer.end())
                tokens.push_back(x);

            EXPECT_TRUE(false)
                << tokens.size() << "  " << tokens2str(tokens);
        );

        lexer.reset();
    }
}
