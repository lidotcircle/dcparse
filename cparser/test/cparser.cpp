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
    parser.setDebugStream(std::cout);

    vector<string> test_cases = {
        "int a = 0;",
    };

    for (auto t: test_cases) {
        EXPECT_NO_THROW(
            for (auto c: t) {
                parser.feed(c);
            }

            auto tunit = parser.end();
            EXPECT_NE(tunit, nullptr);
        );

        parser.reset();
    }
}

TEST(should_reject, CParserLexer) {
    CLexerParser parser;

    vector<string> test_cases = {
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

