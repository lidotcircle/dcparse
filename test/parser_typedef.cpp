#include "parser/parser.h"
#include <gtest/gtest.h>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>
using namespace std;


using ParserChar = DCParser::ParserChar;

struct TokenSSV : public LexerToken
{
    TokenSSV(TextRange info) : LexerToken(info)
    {}
    virtual string str() = 0;
};

struct TokenID : public TokenSSV
{
    string id;
    TokenID(string id) : TokenSSV(TextRange()), id(id)
    {}
    virtual string str() override
    {
        return id;
    }
};


/*         test grammar
 *
 * Tokens: ID, TYPEDEF, +, ;, (, )
 *
 * UNIT -> Type
 * UNIT -> Stat
 * UNIT -> Decl
 * UNIT -> UNIT Type
 * UNIT -> UNIT Stat
 * UNIT -> UNIT Decl
 *
 * Type -> ID                        decision: reduce when ID is a type
 * Expr -> ID
 * Expr -> Expr optional(ID) ID      decision: reduce when ID is a type
 * Expr -> Expr + Expr
 * Expr -> ( Type ) Expr
 * Expr -> ( Expr ) Expr
 * Decl -> Type ID ;
 *
 * Stat -> Expr ;
 * Typedef -> TYPEDEF ID ;
 */


#define NTOKENS                                                                                    \
    TENTRY(TYPEDEF, "d")                                                                           \
    TENTRY(PLUS, "+")                                                                              \
                                                                                                   \
    TENTRY(LPAREN, "(")                                                                            \
    TENTRY(RPAREN, ")")                                                                            \
                                                                                                   \
    TENTRY(SEMICOLON, ";")

#define TENTRY(n, s)                                                                               \
    struct Token##n : public TokenSSV                                                              \
    {                                                                                              \
        Token##n(TextRange info) : TokenSSV(info)                                                  \
        {}                                                                                         \
        Token##n() : TokenSSV(TextRange())                                                         \
        {}                                                                                         \
        virtual string str() override                                                              \
        {                                                                                          \
            return s;                                                                              \
        }                                                                                          \
    };
NTOKENS
#undef TENTRY

#define MK(t) make_shared<Token##t>()
#define MD(id) make_shared<TokenID>(id)
#define TI(t) CharInfo<Token##t>()

struct NonTermStr : public NonTerminal
{
    string _str;
    virtual string str()
    {
        return _str;
    }
    NonTermStr(string s) : _str(s)
    {}
};

#define NNONTERM                                                                                   \
    TENTRY(Unit)                                                                                   \
    TENTRY(Decl)                                                                                   \
    TENTRY(Expr)                                                                                   \
    TENTRY(Type)                                                                                   \
    TENTRY(Stat)                                                                                   \
    TENTRY(Typedef)

#define TENTRY(n)                                                                                  \
    struct NonTerm##n : public NonTermStr                                                          \
    {                                                                                              \
        NonTerm##n(string s) : NonTermStr(s)                                                       \
        {}                                                                                         \
    };
NNONTERM
#undef TENTRY

#define NI(t) CharInfo<NonTerm##t>()

static string c2s(dchar_t cr)
{
    if (!cr)
        return "";

    auto p1 = dynamic_pointer_cast<TokenSSV>(cr);
    auto p2 = dynamic_pointer_cast<NonTermStr>(cr);

    if (p1)
        return p1->str();
    if (p2)
        return p2->str();
    throw runtime_error("NOPE");
}
static string cs2s(vector<dchar_t> crs)
{
    string ret;
    for (auto c : crs) {
        if (c)
            ret += c2s(c);
    }
    return ret;
}

class TypedefParserTest : public ::testing::Test
{
  protected:
    DCParser parser;

    void SetUp() override
    {
        static set<string> types;
        parser(
            NI(Type),
            {TI(ID)},
            [](auto, auto ts) {
                assert(ts.size() == 1);
                auto id = dynamic_pointer_cast<TokenID>(ts[0]);
                return make_shared<NonTermType>("t-" + cs2s(ts));
            },
            RuleAssocitiveLeft,
            make_shared<RuleDecisionFunction>([](auto, auto& v, auto&) {
                EXPECT_EQ(v.size(), 1);
                auto t = v.front();
                auto k = dynamic_pointer_cast<TokenID>(t);
                return types.find(k->id) != types.end();
            }));

        parser.dec_priority();

        parser(
            NI(Expr),
            {NI(Expr), ParserChar::beOptional(TI(ID)), TI(ID)},
            [](auto, auto ts) {
                assert(ts.size() == 3);
                auto id = dynamic_pointer_cast<TokenID>(ts[0]);
                return make_shared<NonTermExpr>(cs2s(ts));
            },
            RuleAssocitiveRight,
            make_shared<RuleDecisionFunction>([](auto, auto& v, auto&) {
                EXPECT_TRUE(v.size() == 2 || v.size() == 3);

                auto t = v.back();
                auto k = dynamic_pointer_cast<TokenID>(t);
                if (types.find(k->id) != types.end())
                    return true;

                if (v.size() == 3 && v[1]) {
                    auto t = v[1];
                    auto k = dynamic_pointer_cast<TokenID>(t);
                    return types.find(k->id) != types.end();
                }

                return false;
            }));

        parser(NI(Expr), {TI(ID)}, [](auto, auto ts) {
            assert(ts.size() == 1);
            return make_shared<NonTermExpr>("e-" + cs2s(ts));
        });

        parser(NI(Expr), {NI(Expr), TI(PLUS), NI(Expr)}, [](auto, auto ts) {
            assert(ts.size() == 3);
            return make_shared<NonTermExpr>(cs2s(ts));
        });

        parser(NI(Expr), {TI(LPAREN), NI(Type), TI(RPAREN), NI(Expr)}, [](auto, auto ts) {
            assert(ts.size() == 4);
            return make_shared<NonTermExpr>(cs2s(ts));
        });

        parser(NI(Expr), {TI(LPAREN), NI(Expr), TI(RPAREN), NI(Expr)}, [](auto, auto ts) {
            assert(ts.size() == 4);
            return make_shared<NonTermExpr>(cs2s(ts));
        });

        parser(NI(Stat), {NI(Expr), TI(SEMICOLON)}, [](auto, auto ts) {
            assert(ts.size() == 2);
            return make_shared<NonTermStat>(cs2s(ts));
        });

        parser(NI(Decl), {NI(Type), TI(ID), TI(SEMICOLON)}, [](auto, auto ts) {
            assert(ts.size() == 3);
            return make_shared<NonTermDecl>(cs2s(ts));
        });

        parser(NI(Typedef), {TI(TYPEDEF), TI(ID), TI(SEMICOLON)}, [](auto, auto ts) {
            assert(ts.size() == 3);
            auto id = dynamic_pointer_cast<TokenID>(ts[1]);
            types.insert(id->id);
            return make_shared<NonTermTypedef>(cs2s(ts));
        });


        parser.dec_priority();

        parser(NI(Unit), {NI(Unit), NI(Typedef)}, [](auto, auto ts) {
            assert(ts.size() == 2);
            return make_shared<NonTermUnit>(cs2s(ts));
        });

        parser(NI(Unit), {NI(Unit), NI(Stat)}, [](auto, auto ts) {
            assert(ts.size() == 2);
            return make_shared<NonTermUnit>(cs2s(ts));
        });

        parser(NI(Unit), {NI(Unit), NI(Decl)}, [](auto, auto ts) {
            assert(ts.size() == 2);
            return make_shared<NonTermUnit>(cs2s(ts));
        });

        parser(NI(Unit), {NI(Typedef)}, [](auto, auto ts) {
            assert(ts.size() == 1);
            return make_shared<NonTermUnit>(cs2s(ts));
        });

        parser(NI(Unit), {NI(Stat)}, [](auto, auto ts) {
            assert(ts.size() == 1);
            return make_shared<NonTermUnit>(cs2s(ts));
        });

        parser(NI(Unit), {NI(Decl)}, [](auto, auto ts) {
            assert(ts.size() == 1);
            return make_shared<NonTermUnit>(cs2s(ts));
        });

        parser.add_start_symbol(NI(Unit).id);

        parser.generate_table();
    }
};


TEST_F(TypedefParserTest, beyongCF)
{
    vector<std::pair<vector<dctoken_t>, string>> test_cases = {
        {{
             MD("id"),
             MK(SEMICOLON),
         },
         "e-id;"},
        {{
             MD("int"),
             MK(SEMICOLON),
         },
         "e-int;"},
        {{MK(TYPEDEF),
          MD("int"),
          MK(SEMICOLON),
          MK(LPAREN),
          MD("int"),
          MK(RPAREN),
          MD("hello"),
          MK(SEMICOLON)},
         "dint;(t-int)e-hello;"},
        {{MK(TYPEDEF), MD("long"), MK(SEMICOLON), MD("long"), MD("hello"), MK(SEMICOLON)},
         "dlong;t-longhello;"},
        {{MK(TYPEDEF), MD("float"), MK(SEMICOLON), MD("kv"), MD("float"), MK(SEMICOLON)},
         "dfloat;e-kvfloat;"},
        {{MK(TYPEDEF),
          MD("double"),
          MK(SEMICOLON),
          MD("kv"),
          MD("double"),
          MD("xm"),
          MK(SEMICOLON)},
         "ddouble;e-kvdoublexm;"},
        {{MK(TYPEDEF),
          MD("complex"),
          MK(SEMICOLON),
          MD("kv"),
          MD("xm"),
          MD("complex"),
          MK(SEMICOLON)},
         "dcomplex;e-kvxmcomplex;"},
    };

    for (auto& t : test_cases) {
        auto& ts = t.first;
        auto& rz = t.second;
        auto rt = parser.parse(ts.begin(), ts.end());
        EXPECT_NE(rt, nullptr);
        auto stat = dynamic_pointer_cast<NonTermUnit>(rt);
        EXPECT_NE(stat, nullptr);
        EXPECT_EQ(stat->str(), rz);
        parser.reset();
    }
}
