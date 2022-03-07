#include <gtest/gtest.h>
#include "c_token.h"
#include <iostream>
#include <vector>
#include <string>
using namespace std;


TEST(rulesInit, CLexerBasic) {
    EXPECT_NO_THROW(
        cparser::CLexerUTF8 lexer;
    );
}

string tokens2str(vector<shared_ptr<LexerToken>> tokens) {
    string result;
    for (auto token : tokens)
        result += string(token->charname()) + "[" + to_string(token->position()) + "] ";
    return result;
}

TEST(should_accept, CLexerBasic) {
    cparser::CLexerUTF8 lexer;

    vector<pair<string,size_t>> test_cases = {
        { "/* asdfzxcv  */", 0 },
        { "/* asdfzxcv  */ */", 2 },
        { "/**", 3 },
        { "// asdfzxcv", 0 },
        { "//a\nbc // asdf", 1},
        { "\"int\"", 1 },
        { "\"\"", 1 },
        { "\"\\\"\"", 1 },
        { "\"a\\a\\b\\c\\d\\\"abcd\"", 1 },
        { "if else elll", 3},
        { "int", 1 },
        { "_a _b c d e f", 6 },
        { "[] <::> () {} <%%> . -> ++ -- & * + - ~ ! / % << >> < > <= >= == != ^ | && || ?"
          ": ; ... = *= /= %= += -= <<= >>= &= |= ^= , # %: ##%:%:", 54 },
        { "001 0b01 1234 0x1 'a' 'c' '\\x21' '\\0100'", 8 },
        { "01e+999 22E1 33e-3 02e22 02e22f 02e77F", 6 },
    };

    for (auto ts: test_cases) {
        auto t = ts.first;
        auto n = ts.second;

        vector<shared_ptr<LexerToken>> tokens;
        for (auto c: t) {
            for (auto x: lexer.feed(c))
                tokens.push_back(x);
        }
        for (auto x: lexer.end())
            tokens.push_back(x);

        EXPECT_EQ(n, tokens.size()) << t << " " << n << endl
            << tokens2str(tokens);

        lexer.reset();
    }
}

TEST(keyword, CLexerBasic) {
    cparser::CLexerUTF8 lexer;

    vector<pair<string,size_t>> test_cases = {
#define K_ENTRY(n) { #n, CharID<cparser::TokenKeyword_##n>() },
C_KEYWORD_LIST
#undef K_ENTRY
    };

    for (auto ts: test_cases) {
        auto t = ts.first;
        auto id = ts.second;

        vector<shared_ptr<LexerToken>> tokens;
        for (auto c: t) {
            for (auto x: lexer.feed(c))
                tokens.push_back(x);
        }
        for (auto x: lexer.end())
            tokens.push_back(x);

        ASSERT_EQ(1, tokens.size()) << t << endl
            << tokens2str(tokens);

        auto token = tokens[0];
        EXPECT_EQ(id, token->charid()) << t << endl
            << tokens2str(tokens);

        lexer.reset();
    }
}

#define C_PUNCTUATOR_V_LIST \
    P_ENTRY(LBRACKET, "[") \
    P_ENTRY(LBRACKET, "<:") \
    P_ENTRY(RBRACKET, "]") \
    P_ENTRY(RBRACKET, ":>") \
    P_ENTRY(LPAREN, "(") \
    P_ENTRY(RPAREN, ")") \
    P_ENTRY(LBRACE, "{") \
    P_ENTRY(LBRACE, "<%") \
    P_ENTRY(RBRACE, "}") \
    P_ENTRY(RBRACE, "%>") \
    P_ENTRY(DOT, ".") \
    P_ENTRY(PTRACCESS, "->") \
    P_ENTRY(PLUSPLUS, "++") \
    P_ENTRY(MINUSMINUS, "--") \
    P_ENTRY(REF, "&") \
    P_ENTRY(MULTIPLY, "*") \
    P_ENTRY(PLUS, "+") \
    P_ENTRY(MINUS, "-") \
    P_ENTRY(BIT_NOT, "~") \
    P_ENTRY(LOGIC_NOT, "!") \
    P_ENTRY(DIVISION, "/") \
    P_ENTRY(REMAINDER, "%") \
    P_ENTRY(LEFT_SHIFT, "<<") \
    P_ENTRY(RIGHT_SHIFT, ">>") \
    P_ENTRY(LESS_THAN, "<") \
    P_ENTRY(GREATER_THAN, ">") \
    P_ENTRY(LESS_EQUAL, "<=") \
    P_ENTRY(GREATER_EQUAL, ">=") \
    P_ENTRY(EQUAL, "==") \
    P_ENTRY(NOT_EQUAL, "!=") \
    P_ENTRY(BIT_XOR, "^") \
    P_ENTRY(BIT_OR, "|") \
    P_ENTRY(LOGIC_AND, "&&") \
    P_ENTRY(LOGIC_OR, "||") \
    P_ENTRY(QUESTION, "?") \
    P_ENTRY(COLON, ":") \
    P_ENTRY(SEMICOLON, ";") \
    P_ENTRY(DOTS, "...") \
    P_ENTRY(ASSIGN, "=") \
    P_ENTRY(MULTIPLY_ASSIGN, "*=") \
    P_ENTRY(DIVISION_ASSIGN, "/=") \
    P_ENTRY(REMAINDER_ASSIGN, "%=") \
    P_ENTRY(PLUS_ASSIGN, "+=") \
    P_ENTRY(MINUS_ASSIGN, "-=") \
    P_ENTRY(LEFT_SHIFT_ASSIGN, "<<=") \
    P_ENTRY(RIGHT_SHIFT_ASSIGN, ">>=") \
    P_ENTRY(BIT_AND_ASSIGN, "&=") \
    P_ENTRY(BIT_OR_ASSIGN, "|=") \
    P_ENTRY(BIT_XOR_ASSIGN, "^=") \
    P_ENTRY(COMMA, ",") \
    P_ENTRY(SHARP, "#") \
    P_ENTRY(SHARP, "#") \
    P_ENTRY(DOUBLE_SHARP, "##") \
    P_ENTRY(DOUBLE_SHARP, "%:%:")

TEST(punctuator, CLexerBasic) {
    cparser::CLexerUTF8 lexer;

    vector<pair<string,size_t>> test_cases = {
#define P_ENTRY(n, r) { r, CharID<cparser::TokenPunc##n>() },
C_PUNCTUATOR_V_LIST
#undef P_ENTRY
    };

    for (auto ts: test_cases) {
        auto t = ts.first;
        auto id = ts.second;

        vector<shared_ptr<LexerToken>> tokens;
        for (auto c: t) {
            for (auto x: lexer.feed(c))
                tokens.push_back(x);
        }
        for (auto x: lexer.end())
            tokens.push_back(x);

        ASSERT_EQ(1, tokens.size()) << t << endl
            << tokens2str(tokens);

        auto token = tokens[0];
        EXPECT_EQ(id, token->charid()) << t << endl
            << tokens2str(tokens);

        lexer.reset();
    }
}

TEST(shoud_reject, CLexerBasic) {
    cparser::CLexerUTF8 lexer;

    vector<string> test_cases = {
        "$",
    };

    for (auto ts: test_cases) {

        EXPECT_ANY_THROW(
            vector<shared_ptr<LexerToken>> tokens;
            for (auto c: ts) {
                for (auto x: lexer.feed(c))
                    tokens.push_back(x);
            }
            for (auto x: lexer.end())
                tokens.push_back(x);

            EXPECT_TRUE(false)
                << tokens.size() << "  " << tokens2str(tokens);
        );

        lexer.reset();
    }
}
