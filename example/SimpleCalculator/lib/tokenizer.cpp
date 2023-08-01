#include "scalc/token.h"
#include "scalc/parser.h"
#include "lexer/lexer_rule_regex.hpp"
#include <string>
using namespace std;

using token_t = CalcLexer::token_t;
inline static auto s2u(string str) { return UTF8Decoder::strdecode(str); };
inline static auto u2s(std::vector<int> cps) { return UTF8Encoder::strencode(cps.begin(), cps.end()); }

#define KEYWORD_TOKEN \
    TENTRY(PLUS,         "\\+") \
    TENTRY(MINUS,        "\\-") \
    TENTRY(MULTIPLY,     "\\*") \
    TENTRY(DIVISION,     "/") \
    TENTRY(REMAINDER,    "%") \
    TENTRY(CARET,        "\\^") \
    TENTRY(PLUSPLUS,     "\\+\\+") \
    TENTRY(MINUSMINUS,   "\\-\\-") \
    \
    TENTRY(EQUAL,        "==") \
    TENTRY(NOTEQUAL,     "!=") \
    TENTRY(GREATERTHAN,  ">") \
    TENTRY(LESSTHAN,     "<") \
    TENTRY(GREATEREQUAL, ">=") \
    TENTRY(LESSEQUAL,    "<=") \
    \
    TENTRY(ASSIGNMENT,   "=") \
    \
    TENTRY(LPAREN,       "\\(") \
    TENTRY(RPAREN,       "\\)") \
    TENTRY(LBRACE,       "\\{") \
    TENTRY(RBRACE,       "\\}") \
    \
    TENTRY(IF,           "if") \
    TENTRY(ELSE,         "else") \
    TENTRY(FOR,          "for") \
    TENTRY(WHILE,        "while") \
    TENTRY(FUNCTION,     "function") \
    TENTRY(RETURN,       "return") \
    \
    TENTRY(COMMA,        ",") \
    TENTRY(SEMICOLON,    ";") \
    TENTRY(NEWLINE,      "\n")


CalcLexer::CalcLexer()
{
    auto& lexer = *static_cast<Lexer<int>*>(this);

    lexer(
        std::make_unique<LexerRuleRegex<int>>(
            s2u("/\\*(!\\*/)\\*/"),
            [](auto str, auto info) {
                return nullptr;
            }, false, true)
    );

    lexer(
        std::make_unique<LexerRuleRegex<int>>(
            s2u("//[^\n]*\n"),
            [](auto str, auto info) {
                return nullptr;
            }, false, true)
    );

    lexer.dec_priority_major();

#define TENTRY(cn, reg) \
    lexer( \
        std::make_unique<LexerRuleRegex<int>>( \
            s2u(reg), \
            [](auto str, auto info) { \
                return std::make_shared<Token##cn>(info); \
            }) \
    );
KEYWORD_TOKEN
#undef TENTRY

    lexer.dec_priority_minor();

    lexer(
        std::make_unique<LexerRuleRegex<int>>(
            s2u("[a-zA-Z_][a-zA-Z0-9_]*"),
            [](auto str, auto info) {
            return std::make_shared<TokenID>(
                    string(u2s(str)),
                    info);
        })
    );


    lexer(
        std::make_unique<LexerRuleRegex<int>>(
            s2u("[\\-+]?([0-9]*[.])?[0-9]+([eE][\\-+]?[0-9]+)?"),
            [](auto str, auto info) {
            size_t len = 0;
            auto val = std::stod(string(str.begin(),str.end()), &len);
            assert(len == str.size());
            return std::make_shared<TokenNUMBER>(val, info);
        }, true, false,
            [](auto last) {
                static const CalcParser calcparser(false);
                static const auto possible_prev = calcparser.prev_possible_token_of(CharID<TokenNUMBER>());

                if (!last.has_value() || dynamic_pointer_cast<TokenNEWLINE>(last.value()))
                    return true;

                auto last_token = last.value();
                assert(last_token);
                const auto id = last_token->charid();
                return possible_prev.find(id) != possible_prev.end();
            }
        )
    );

    lexer.dec_priority_major();

    // space, tab, newline
    lexer(
        std::make_unique<LexerRuleRegex<int>>(
            s2u("( |\t|\r)+"),
            [](auto str, auto info) {
            return nullptr;
        })
    );

    lexer.reset();
}

vector<token_t> CalcLexer::feed(char c)
{
    auto cx = this->m_decoder.decode(c);
    if (cx.presented())
        return this->feed_char(cx.getval());
    else
        return vector<token_t>();
}

vector<token_t> CalcLexer::end()
{
    return this->feed_end();
}

void CalcLexer::reset()
{
    this->m_decoder = UTF8Decoder();
    Lexer<int>::reset();
}
