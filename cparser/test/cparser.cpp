#include <gtest/gtest.h>
#include "c_parser.h"
#include "c_lexer_parser.h"
#include <iostream>
#include <string>
#include <vector>
using namespace cparser;
using namespace std;


TEST(RuleTransitionTable, CParserBasic) {
    EXPECT_NO_THROW(
        cparser::CParser parser;
    );
}

TEST(should_accpet, CParserLexer) {
    CLexerParser parser;
    // parser.setDebugStream(std::cout);

    vector<string> test_cases = {
        "int;",
        "int a = a;",
        "int b = 1;",
        "int c = 1.11;",
        "int c = ( 1.11 );",
        "int c = a[b], d = a();",
        "int c = a[b], d = a(), a = q(a,b,c), e = a.b->d, e = f++, z = x--;",
        "int c[], f[22], g[static const 2];",
        "int (*c)(), (*f)(int), (*g)(int, int);",
        "int (*c(int))();",
        "typedef int hello; hello a; hello hello;",
        "int * const * const volatile a = 22 * 44 * 88;",
        "static const inline a = 22 * 44 + 55;",
        "int main();",
    };

    for (auto t: test_cases) {
        ASSERT_NO_THROW(
            for (auto c: t) {
                parser.feed(c);
            }

            auto tunit = parser.end();
            EXPECT_NE(tunit, nullptr);
        ) << t;

        parser.reset();
    }
}

TEST(should_reject, CParserLexer) {
    CLexerParser parser;

    vector<string> test_cases = {
        "",
    };

    for (auto t: test_cases) {

        EXPECT_ANY_THROW(
            for (auto c: t)
                parser.feed(c);
            auto tunit = parser.end();
        );

        parser.reset();
    }

}

