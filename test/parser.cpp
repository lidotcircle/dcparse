#include <gtest/gtest.h>
#include <vector>
#include <tuple>
#include <stdexcept>
#include "parser/parser.h"
using namespace std;


struct TokenSSV: public LexerToken {
    TokenSSV(LexerToken::TokenInfo info): LexerToken(info) {}
    virtual string str() = 0;
};

#define NTOKENS \
    TENTRY(NUMBER, "n") \
    TENTRY(ID, "i") \
    \
    TENTRY(PLUS, "+") \
    TENTRY(MINUS, "-") \
    TENTRY(MULTIPLY, "*") \
    TENTRY(DIVIDE, "/") \
    \
    TENTRY(ASSIGNMENT, "=") \
    \
    TENTRY(LPAREN, "(") \
    TENTRY(RPAREN, ")") \
    \
    TENTRY(SEMICOLON, ";")

#define TENTRY(n, s) \
    struct Token##n: public TokenSSV { \
        Token##n(LexerToken::TokenInfo info): \
            TokenSSV(info) {} \
        Token##n(): TokenSSV(TokenInfo()) {} \
        virtual string str() override { return s; } \
    };
NTOKENS
#undef TENTRY

#define MK(t) make_shared<Token##t>()
#define TI(t) CharID<Token##t>()

struct NonTermStr: public NonTerminal {
    string _str;
    virtual string str() { return _str; }
    NonTermStr(string s): _str(s) {}
};

#define NNONTERM \
    TENTRY(EXPR) \
    TENTRY(OP1) \
    TENTRY(OP2) \
    TENTRY(STATEMENT)

#define TENTRY(n) \
    struct NonTerm##n: public NonTermStr { \
        NonTerm##n(string s): NonTermStr(s) {} \
    };
NNONTERM
#undef TENTRY

#define NI(t) CharID<NonTerm##t>()

static string c2s(dchar_t cr)
{
    auto p1 = dynamic_pointer_cast<TokenSSV>(cr);
    auto p2 = dynamic_pointer_cast<NonTermStr>(cr);

    if (p1) return p1->str();
    if (p2) return p2->str();
    throw runtime_error("NOPE");
}
static string cs2s(vector<dchar_t> crs)
{
    string ret;
    for (auto c: crs) ret += c2s(c);
    return ret;
}


class ExprParserTest: public ::testing::Test {
protected:
    DCParser parser;

    void SetUp() override {
        parser( NI(EXPR),
                { NI(EXPR), NI(OP2), NI(EXPR) },
                [](auto, auto ts) {
                    assert(ts.size() == 3);
                    return make_shared<NonTermEXPR>("(" + cs2s(ts) + ")");
                } );

        parser ( NI(OP2),
                { TI(MULTIPLY) },
                [](auto, auto ts) {
                    assert(ts.size() == 1);
                    return make_shared<NonTermOP2>(cs2s(ts));
                } );

        parser ( NI(OP2),
                { TI(DIVIDE) },
                [](auto, auto ts) {
                    assert(ts.size() == 1);
                    return make_shared<NonTermOP2>(cs2s(ts));
                } );

        parser.dec_priority();

        parser( NI(EXPR),
                { NI(EXPR), NI(OP1), NI(EXPR) },
                [](auto, auto ts) {
                    assert(ts.size() == 3);
                    return make_shared<NonTermEXPR>("(" + cs2s(ts) + ")");
                } );

        parser ( NI(OP1),
                { TI(PLUS) },
                [](auto, auto ts) {
                    assert(ts.size() == 1);
                    return make_shared<NonTermOP1>(cs2s(ts));
                } );

        parser ( NI(OP1),
                { TI(MINUS) },
                [](auto, auto ts) {
                    assert(ts.size() == 1);
                    return make_shared<NonTermOP1>(cs2s(ts));
                } );

        parser.dec_priority();

        parser( NI(EXPR),
                { NI(EXPR), TI(ASSIGNMENT), NI(EXPR) },
                [](auto, auto ts) {
                    assert(ts.size() == 3);
                    return make_shared<NonTermEXPR>("(" + cs2s(ts) + ")");
                }, RuleAssocitiveRight);

        parser.dec_priority();

        parser( NI(EXPR),
                { TI(LPAREN), NI(EXPR), TI(RPAREN) },
                [](auto, auto& ts) {
                    assert(ts.size() == 3);
                    return make_shared<NonTermEXPR>(cs2s(ts));
                } );

        parser.dec_priority();

        parser( NI(EXPR),
                { TI(ID) },
                [](auto, auto ts) {
                    assert(ts.size() == 1);
                    return make_shared<NonTermEXPR>(cs2s(ts));
                } );

        parser( NI(EXPR),
                { TI(NUMBER) },
                [](auto, auto ts) {
                    assert(ts.size() == 1);
                    return make_shared<NonTermEXPR>(cs2s(ts));
                } );

        parser.dec_priority();

        parser( NI(STATEMENT),
                { NI(EXPR), TI(SEMICOLON) },
                [] (auto, auto& ts) {
                    assert(ts.size() == 2);
                    return make_shared<NonTermSTATEMENT>(cs2s(ts));
                } );

        parser.add_start_symbol(NI(STATEMENT));

        parser.generate_table();
    }
};


TEST_F(ExprParserTest, ID_NUMBER) {
    vector<std::pair<vector<dctoken_t>,string>> test_cases = {
        { { MK(ID), MK(SEMICOLON), }, "i;" },
        { { MK(NUMBER), MK(SEMICOLON), }, "n;" },
        { { MK(ID), MK(PLUS), MK(NUMBER), MK(SEMICOLON), }, "(i+n);" },
        { { MK(ID), MK(MINUS), MK(NUMBER), MK(SEMICOLON), }, "(i-n);" },
        { { MK(ID), MK(MULTIPLY), MK(NUMBER), MK(SEMICOLON), }, "(i*n);" },
        { { MK(ID), MK(DIVIDE), MK(NUMBER), MK(SEMICOLON), }, "(i/n);" },
        { { MK(ID), MK(PLUS), MK(NUMBER), MK(PLUS), MK(ID), MK(SEMICOLON), }, "((i+n)+i);" },
        { { MK(ID), MK(ASSIGNMENT), MK(NUMBER), MK(ASSIGNMENT), MK(ID), MK(SEMICOLON), }, "(i=(n=i));" },
        { { MK(ID), MK(PLUS), MK(NUMBER), MK(MULTIPLY), MK(ID), MK(SEMICOLON), }, "(i+(n*i));" },
        { { MK(LPAREN), MK(ID), MK(PLUS), MK(NUMBER), MK(RPAREN), MK(MULTIPLY), MK(ID), MK(SEMICOLON), }, "(((i+n))*i);" },
    };

    for (auto& t: test_cases) {
        auto& ts = t.first;
        auto& rz = t.second;
        auto rt = parser.parse(ts.begin(), ts.end());
        EXPECT_NE(rt, nullptr);
        auto stat = dynamic_pointer_cast<NonTermSTATEMENT>(rt);
        EXPECT_NE(stat, nullptr);
        EXPECT_EQ(stat->str(), rz);
        parser.reset();
    }
}

