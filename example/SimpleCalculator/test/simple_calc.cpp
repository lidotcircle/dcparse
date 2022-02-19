#include <gtest/gtest.h>
#include "scalc/lexer_parser.h"
#include <string>
using namespace std;


TEST(SimpleCalculator, AcceptText) {
    vector<string> test_cases = {
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

