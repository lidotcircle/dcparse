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
        "int main() {}",
        "int main() { int a = 2; }",
        "int hux(a) int a; { return a; }",
        "int main() { for(int i = 0; i < 10; i++) { } }",
        "int main() { a: m = 2; }",
        "int main(int argc, char* argv[]) {}",

        "char;",
        "signed char;",
        "unsigned char;",
        "short;",
        "signed short;",
        "short int;",
        "signed short int;",
        "unsigned short;",
        "unsigned short int;",
        "int;",
        "signed;",
        "signed int;",
        "unsigned;",
        "unsigned int;",
        "long;",
        "signed long;",
        "long int;",
        "signed long int;",
        "unsigned long;",
        "long unsigned;",
        "unsigned long int;",
        "long long;",
        "signed long long;",
        "long long int;",
        "signed long long int;",
        "unsigned long long;",
        "unsigned long long int;",
        "float;",
        "double;",
        "long double;",
        "_Bool;",

        "int primary_expr = a;",
        "int primary_expr = 22;",
        "int primary_expr = 22.2;",
        "int primary_expr = \"hello world\";",
        "int primary_expr = ( a );",

        "int posfix_expr = ( expr );",
        "int posfix_expr = ( expr )[a];",
        "int posfix_expr = ( expr )();",
        "int posfix_expr = ( expr )(a, b, c);",
        "int posfix_expr = ( expr ).a;",
        "int posfix_expr = ( expr )->a;",
        "int posfix_expr = ( expr )++;",
        "int posfix_expr = ( expr )--;",
        "int posfix_expr = ( int ){ .a = 2, };",
        "int posfix_expr = ( int ){ [a] = 2, };",
        "int posfix_expr = ( int ){ .a = 2, 3, 4 };",

        "int unary_expr = ++a;",
        "int unary_expr = --a;",
        "int unary_expr = +a;",
        "int unary_expr = -a;",
        "int unary_expr = !a;",
        "int unary_expr = ~a;",
        "int unary_expr = *a;",
        "int unary_expr = &a;",
        "int unary_expr = sizeof a;",
        "int unary_expr = sizeof(a);",
        "int unary_expr = sizeof(int);",
        "int unary_expr = sizeof(long int*);",
        "int unary_expr = sizeof(const long int*);",
        "int unary_expr = sizeof(const long int (*)(int));",

        "int cast_expr = (int)a;",
        "int cast_expr = (int*)a;",
        "int cast_expr = (int* const)a;",
        "int cast_expr = (int* const*)a;",

        "int multiplicative_expr = a * b;",
        "int multiplicative_expr = a / b;",
        "int multiplicative_expr = a % b;",

        "int additive_expr = a + b;",
        "int additive_expr = a - b;",

        "int shift_expr = a << b;",
        "int shift_expr = a >> b;",

        "int relational_expr = a < b;",
        "int relational_expr = a > b;",
        "int relational_expr = a <= b;",
        "int relational_expr = a >= b;",

        "int equality_expr = a == b;",
        "int equality_expr = a != b;",

        "int and_expr = a & b;",

        "int xor_expr = a ^ b;",

        "int or_expr = a | b;",

        "int logical_and_expr = a && b;",

        "int logical_or_expr = a || b;",

        "int conditional_expr = a ? b : c;",
        "int conditional_expr = a ? b : c ? d : e;",

        "int assignment_expr = a = b;",
        "int assignment_expr = a += b;",
        "int assignment_expr = a -= b;",
        "int assignment_expr = a *= b;",
        "int assignment_expr = a /= b;",
        "int assignment_expr = a %= b;",
        "int assignment_expr = a <<= b;",
        "int assignment_expr = a >>= b;",
        "int assignment_expr = a &= b;",
        "int assignment_expr = a ^= b;",
        "int assignment_expr = a |= b;",

        "int expr = a, b, c;",

        "struct sct;",
        "struct sct {};",
        "struct sct { ; };",
        "struct sct { int a; };",
        "struct sct { const int a; };",
        "struct sct { const int a; int a:4; int b:4; };",
        "struct sct { const int:4; int a:4; int b:4; };",
        "struct sct { const int **a; };",
        "struct sct { const int (*a)(); };",
        "struct sct { const int (*a)(int, ...); };",

        "union un ;",
        "union un {};",

        "enum em { };",
        "enum em { , };",
        "enum em { a = 1 };",
        "enum em { a = 1, b, d };",
        "enum em { a = 1, b, d, };",

        "int a = { };",
        "int a = { .a = 10 };",
        "int a = { [a][b] = 10 };",

        "int main() { label: return; }",
        "int main() { switch (a) { case 1: break; default: break; } }",
        "int main() { switch (a) { case 1: break; default: break; } { int a; } }",
        "int main() { { if (a) a = b; if (10) return; else return; } }",
        "int main() { while(i) i--; }",
        "int main() { for(i=0;i;i--) i--; }",
        "int main() { for(int i=0, j=0;i;i--) i--; }",
        "int main() { goto a; continue; break; return; return a; }",
        "struct { long long a; } a;",

        "_Static_assert(0, \"hello\");",
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
        "typedef",
        " { int a; } ",
        "const int * int;",
        "int hello*;",
        "struct;",
        "enum;",
        "struct str long;",
        "long struct str;",
        "long long long;",
        "long long long unsigned;",
        "struct { long long long a; } a;",
        "int hux() int b; { return a; }",
        "int hux(a) int b; { return a; }",
        "int hux(a, int b) int a; { return a; }",
        "int hx(_Static_assert(0, \"hello\"););",
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

