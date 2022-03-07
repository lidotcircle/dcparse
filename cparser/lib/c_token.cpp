#include "c_token.h"
#include "lexer/lexer_rule_regex.hpp"
#include "dcutf8.h"
#include <stdexcept>
#include <algorithm>
#include <limits>
using namespace std;

inline static auto s2u(string str) { return UTF8Decoder::strdecode(str); };
inline static auto u2s(std::vector<int> cps) { return UTF8Encoder::strencode(cps.begin(), cps.end()); }
#define INTEGER_SUFFIX_REGEX "([uU][lL]?|[uU](ll|LL)|[lL][uU]?|(ll|LL)[uU]?)?"

struct integer_suffix_info {
    bool is_unsigned;
    size_t bit_width;
    size_t suffix_len;
};
static constexpr size_t max_integer_size = sizeof(long long);
static constexpr size_t long_integer_size = sizeof(long);
static constexpr size_t long_long_integer_size = sizeof(long long);
static constexpr size_t int_integer_size = sizeof(int);

static integer_suffix_info parse_integer_suffix(const string& str)
{
    integer_suffix_info info;
    info.is_unsigned = false;
    info.bit_width = 8 * max_integer_size;
    info.suffix_len = 0;

    for (auto it = str.rbegin();it != str.rend();it++) {
        char c = *it;

        switch (c) {
        case 'u':
        case 'U':
            info.is_unsigned = true;
            info.suffix_len++;
            continue;
        case 'l':
        case 'L':
            if (info.bit_width == 8 * max_integer_size)
                info.bit_width = 8 * long_integer_size;
            else
                info.bit_width = 8 * long_long_integer_size;
            info.suffix_len++;
            continue;
        default:
            break;
        }

        break;
    }

    return info;
}

static bool string_start_with(const string& str, const string& prefix)
{
    return str.compare(0, prefix.size(), prefix) == 0;
}

static cparser::TokenConstantInteger handle_integer_str(string str, LexerToken::TokenInfo tinfo)
{
    assert(str.size() > 0);

    auto info = parse_integer_suffix(str);
    str = str.substr(0, str.size() - info.suffix_len);

    unsigned long long value = 0;
    const bool is_hex = 
        string_start_with(str, "0x") 
        || string_start_with(str, "0X")
        || std::any_of(
            str.begin(), str.end(), 
            [](char c) { return ('a' <= c && c <= 'f') || ('A' <= c && c <= 'F'); });
    assert(str.size() > 0);

    if (is_hex) 
    {
        if (string_start_with(str, "0x") ||
                string_start_with(str, "0X"))
        {
            str = str.substr(2);
            assert(str.size() > 0);
        }

        for (auto it = str.begin();it != str.end();it++) {
            const auto old_value = value;
            char c = *it;
            if (('0' <= c && c <= '9') || ('a' <= c && c <= 'f') || ('A' <= c && c <= 'F')) {
                value = value * 16 + (c <= '9' ? c - '0' : (c <= 'F' ? c - 'A' + 10 : c - 'a' + 10));
            } else {
                assert(false && "bad hex string");
            }
            if (value < old_value)
                throw std::runtime_error("integer literal overflow");
        }
    }
    else if (string_start_with(str, "0b"))
    {
        str = str.substr(2);
        assert(str.size() > 0);

        for (auto it = str.begin();it != str.end();it++) {
            const auto old_value = value;
            char c = *it;
            if (c == '0' || c == '1') {
                value = value * 2 + (c - '0');
            } else {
                assert(false && "bad binary string");
            }
            if (value < old_value)
                throw std::runtime_error("integer literal overflow");
        }
    }
    else if (string_start_with(str, "0"))
    {
        if (str.size() > 1)
            str = str.substr(1);
        assert(str.size() > 0);

        for (auto it = str.begin();it != str.end();it++) {
            const auto old_value = value;
            char c = *it;
            if (c >= '0' && c <= '7') {
                value = value * 8 + (c - '0');
            } else {
                assert(false && "bad octal string");
            }
            if (value < old_value)
                throw std::runtime_error("integer literal overflow");
        }
    }
    else
    {
        for (auto it = str.begin();it != str.end();it++) {
            const auto old_value = value;
            char c = *it;
            if (c >= '0' && c <= '9') {
                value = value * 10 + (c - '0');
            } else {
                assert(false && "bad decimal string");
            }
            if (value < old_value)
                throw std::runtime_error("integer literal overflow");
        }
    }

    if (value > (unsigned long long)std::numeric_limits<long long>::max())
        info.is_unsigned = true;

     cparser::TokenConstantInteger ret(value, tinfo);
     ret.is_unsigned = info.is_unsigned;
     return ret;
}

static cparser::TokenConstantInteger handle_integer_str(vector<int> str, LexerToken::TokenInfo tinfo)
{
    return handle_integer_str(string(str.begin(), str.end()), tinfo);
}

static cparser::TokenConstantInteger handle_character_str(vector<int> str, LexerToken::TokenInfo tinfo)
{
    assert(str.size() >= 3);
    if (str.front() == 'L')
        str.erase(str.begin());

    assert(str[0] == '\'' && str.back() == '\'');
    str = vector<int>(str.begin() + 1, str.end() - 1);
    if (str.size() > 1 && str.front() != '\\')
        throw std::runtime_error("multi-character character literal");

    if (str.size() == 1)
        return cparser::TokenConstantInteger(str.front(), tinfo);

    int value = 0;
    if (str[1] == 'x') {
        for (size_t i=2;i<str.size();i++) {
            char c = str[i];
            if (('0' <= c && c <= '9') || ('a' <= c && c <= 'f') || ('A' <= c && c <= 'F')) {
                value = value * 16 + (c <= '9' ? c - '0' : (c <= 'F' ? c - 'A' + 10 : c - 'a' + 10));
            } else {
                assert(false && "bad hex string");
            }
        }
    } else {
        for (size_t i=1;i<str.size();i++) {
            char c = str[i];
            if (c >= '0' && c <= '7') {
                value = value * 8 + (c - '0');
            } else {
                assert(false && "bad octal string");
            }
        }
    }

    if (value > 0xff)
        throw std::runtime_error("character literal overflow");

    return cparser::TokenConstantInteger(value, tinfo);
}

namespace cparser {

using token_t = typename CLexer::token_t;


// setup lexer rules when initialization
CLexer::CLexer()
{
    auto& lexer = *this;

    // block comment
    lexer(
        std::make_unique<LexerRuleRegex<int>>(
            s2u("/\\*(!\\*/)\\*/"),
            [](auto str, auto info) {
                return nullptr;
            }, false, true)
    );

    // line comment
    lexer(
        std::make_unique<LexerRuleRegex<int>>(
            s2u("//[^\n]*"),
            [](auto str, auto info) {
                return nullptr;
            })
    );

    // string literal
    lexer(
        std::make_unique<LexerRuleRegex<int>>(
            s2u("L?\"([^\\\\\"\n]|(\\\\[^\n]))*\""),
            [](auto str, auto info) {
            return std::make_shared<TokenStringLiteral>(
                    string(u2s(str)),
                    info);
        })
    );


    // ***********************
    lexer.dec_priority_major();


// keywords
#define K_ENTRY(kw) \
    lexer( \
        std::make_unique<LexerRuleRegex<int>>( \
            s2u(#kw), \
            [](auto str, auto info) { \
                return std::make_shared<TokenKeyword_##kw>(info); \
            }) \
    );
C_KEYWORD_LIST
#undef K_ENTRY

    lexer.dec_priority_minor();

    // identifier
    lexer(
        std::make_unique<LexerRuleRegex<int>>(
            s2u("([a-zA-Z_]|\\\\0[uU][0-9a-fA-F]{4})([a-zA-Z0-9_]|\\\\0[uU][0-9a-fA-F]{4})*"),
            [](auto str, auto info) {
            return std::make_shared<TokenID>(
                    string(u2s(str)),
                    info);
        })
    );


    // ***********************
    lexer.dec_priority_major();


// punctuator
#define P_ENTRY(n, regex) \
    lexer( \
        std::make_unique<LexerRuleRegex<int>>( \
            s2u(regex), \
            [](auto str, auto info) { \
                return std::make_shared<TokenPunc##n>(info); \
            }) \
    );
C_PUNCTUATOR_LIST
#undef P_ENTRY

    lexer.dec_priority_minor();

    // integer literal
    lexer(
        std::make_unique<LexerRuleRegex<int>>(
            s2u("0[0-7]*" INTEGER_SUFFIX_REGEX),
            [](auto str, auto info) {
            return std::make_shared<TokenConstantInteger>(
                    handle_integer_str(str, info));
        })
    );
    lexer(
        std::make_unique<LexerRuleRegex<int>>(
            s2u("0b[01]+" INTEGER_SUFFIX_REGEX),
            [](auto str, auto info) {
            return std::make_shared<TokenConstantInteger>(
                    handle_integer_str(str, info));
        })
    );
    lexer(
        std::make_unique<LexerRuleRegex<int>>(
            s2u("[1-9][0-9]*" INTEGER_SUFFIX_REGEX),
            [](auto str, auto info) {
            return std::make_shared<TokenConstantInteger>(
                    handle_integer_str(str, info));
        })
    );
    lexer.dec_priority_minor();
    lexer(
        std::make_unique<LexerRuleRegex<int>>(
            s2u("(0[xX])?[0-9a-fA-F]+" INTEGER_SUFFIX_REGEX),
            [](auto str, auto info) {
            return std::make_shared<TokenConstantInteger>(
                    handle_integer_str(str, info));
        })
    );
    lexer.dec_priority_minor();
    lexer(
        std::make_unique<LexerRuleRegex<int>>(
            s2u("L?'([^\\\\']+|\\\\.|\\\\0[0-7]*|\\\\x[0-9a-fA-F]+)'" INTEGER_SUFFIX_REGEX),
            [](auto str, auto info) {
            return std::make_shared<TokenConstantInteger>(
                    handle_character_str(str, info));
        })
    );
    lexer(
        std::make_unique<LexerRuleRegex<int>>(
            s2u("[0-9]+[eE][\\+\\-]?[0-9]+[flFL]?"),
            [](auto str, auto info) {
            const long double value = std::stold(u2s(str));
            return std::make_shared<TokenConstantFloat>(value, info);
        })
    );
    lexer(
        std::make_unique<LexerRuleRegex<int>>(
            s2u("((([0-9]+)?\\.[0-9]+)|[0-9]+\\.)([eE][\\+\\-]?[0-9]+)?[flFL]?'"),
            [](auto str, auto info) {
            const long double value = std::stold(u2s(str));
            return std::make_shared<TokenConstantFloat>(value, info);
        })
    );
    // TODO hexadecimal-floating-constant
    // TODO preprocessor

    // ignore space
    lexer.dec_priority_major();
    lexer(
        std::make_unique<LexerRuleRegex<int>>(
            s2u("[ \t\v\f\r\n]+"),
            [](auto str, auto info) {
                return nullptr;
            })
    );

    lexer.reset();
}

vector<token_t> CLexer::feed(int c)
{
    return this->feed_char(c);
}

vector<token_t> CLexer::end()
{
    return this->feed_end();
}

void CLexer::reset()
{
    Lexer<int>::reset();
}

CLexerUTF8::CLexerUTF8() {}

vector<token_t> CLexerUTF8::feed(char c)
{
    auto cx = this->m_decoder.decode(c);

    if (cx.presented())
        return CLexer::feed(cx.getval());
    else
        return {};
}

}
