#include <gtest/gtest.h>
#include "scalc/lexer_parser.h"
#include <sstream>
#include <string>
using namespace std;


TEST(SimpleCalculator, AcceptText) {
    vector<string> test_cases = {
        "a; b;",
        "a = b, c = d, d = a;",
        "a = b; return 1;",
        "function f1() { return a + b; } f1();",
        "a(a);",
        "a(a, b);",
        "c = a(a, b, c);",
        "a = b + c;",
        "a = b - c;",
        "a = b * c;",
        "a = b / c;",
        "a = b % c;",
        "a = b ^ c;",
        "a = func(a + b, c+d);",
        "function f1(a, b) { { c =a + b; return a + b; } }",
    };

    CalcLexerParser lp(false);
    for (auto& t: test_cases) {
        for (auto c: t) lp.feed(c);

        lp.end();
        lp.reset();
    }
}

TEST(SimpleCalculator, OutputTest) {
    vector<pair<string,string>> test_cases = {
        { "a, b = 22 + 33;", "0, 55" },
        { "b + 33;", "88" },
        { "function add(a,b) { return a + b; }", "" },
        { "add(11,22);", "33" },
        { "{ }", "" },
        { "{ h10 = 10; } h10;", "0" },
    };

    CalcLexerParser lp(true);

    auto ctx = lp.getContext();

    for (auto& t: test_cases) {
        ostringstream oss;
        ctx->set_output(&oss);

        auto& in = t.first;
        auto& expected_out = t.second;
        for (auto c: in) lp.feed(c);

        lp.end();

        EXPECT_EQ(oss.str(), expected_out);
        lp.reset();
    }
}
