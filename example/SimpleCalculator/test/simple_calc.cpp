#include <gtest/gtest.h>
#include "scalc/lexer_parser.h"
#include <string>
using namespace std;


TEST(SimpleCalculator, AcceptText) {
    vector<string> test_cases = {
        "a = b;",
    };

    for (auto& t: test_cases) {
        CalcLexerParser lp;
        for (auto c: t) lp.feed(c);

        lp.end();
    }
}

