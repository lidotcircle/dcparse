#include <gtest/gtest.h>
#include "c_lexer_parser.h"
#include <iostream>
#include <string>
#include <vector>
using namespace cparser;
using namespace std;


TEST(should_accpet, CAST) {
    CLexerParser parser;

    vector<string> test_cases = {
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

TEST(should_reject, CAST) {
    CLexerParser parser;

    vector<string> test_cases = {
    };

    for (auto t: test_cases) {

        EXPECT_ANY_THROW(
            for (auto c: t)
                parser.feed(c);
            auto tunit = parser.end();
        ) << t;

        parser.reset();
    }

}
