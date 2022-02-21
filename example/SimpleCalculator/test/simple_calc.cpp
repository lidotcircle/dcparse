#include <gtest/gtest.h>
#include "scalc/lexer_parser.h"
#include <sstream>
#include <string>

#define _USE_MATH_DEFINES
#include <math.h>
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

string to_string2(double d) {
    stringstream ss;
    ss << d;
    return ss.str();
}

TEST(SimpleCalculator, OutputTest) {
    vector<pair<string,string>> test_cases = {
        { "a, b = 22 + 33;", "0, 55" },
        { "b + 33;", "88" },
        { "function add(a,b) { return a + b; }", "" },
        { "add(11,22);", "33" },
        { "{ sin(10); }", "" },
        { "{ h10 = 10; } h10;", "0" },
        { " sin(20); ",   to_string2(sin(20)) },
        { " cos(20); ",   to_string2(cos(20)) },
        { " tan(20); ",   to_string2(tan(20)) },
        { " asin(0.5); ", to_string2(asin(0.5)) },
        { " acos(0.5); ", to_string2(acos(0.5)) },
        { " atan(0.5); ", to_string2(atan(0.5)) },
        { " sqrt(4); ",   to_string2(sqrt(4)) },
        { " abs(-4); ",   to_string2(abs(-4)) },
        { " log(10); ",   to_string2(log(10)) },
        { " log10(10); ", to_string2(log10(10)) },
        { " exp(10); ",   to_string2(exp(10)) },
        { " sum(); ",     to_string2(0) },
        { " sum(10); ",   to_string2(10) },
        { " function above_10(val) { if (val > 10) return 1; else return 0; }", "" }, 
        { " above_10(10); ", "0" },
        { " above_10(11); ", "1" },
        { " above_10(12); ", "1" },
        { " function fibonacci(n) { if (n < 2) return n; else return fibonacci(n- 1) * n; }", "" },
        { " fibonacci(1); ", to_string2(1) },
        { " fibonacci(2); ", to_string2(2) },
        { " fibonacci(3); ", to_string2(6) },
        { " function fibonacci2(n) { ret = 1; for(i=2;i<=n;i++) ret = ret * i; return ret; }", "" },
        { " fibonacci2(1); ", to_string2(1) },
        { " fibonacci2(2); ", to_string2(2) },
        { " fibonacci2(3); ", to_string2(6) },
        { " pi; ",  to_string2(M_PI) },
        { " e; ",   to_string2(M_E) },
        { " inf; ", to_string2(INFINITY) },
        { " nan; ", to_string2(NAN) },
        { " 100+100-20;", "180" },
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

        auto out = oss.str();
        if (!out.empty()) {
            EXPECT_EQ(out.back(), '\n');
            out.pop_back();
        }
        EXPECT_EQ(out, expected_out);
        lp.reset();
    }
}
