#include "lexer/token.h"
#include <string>
#include <typeinfo>
using namespace std;

const char* DChar::charname() const {
    return typeid(*this).name();
}

size_t DChar::charid() const {
    return typeid(*this).hash_code();
}

LexerToken::LexerToken(TokenInfo info): m_info(move(info)) {}

size_t LexerToken::line_number() const {
    return m_info.line_num;
}

size_t LexerToken::column_number() const {
    return m_info.column_num;
}

size_t LexerToken::position() const {
    return m_info.pos;
}

size_t LexerToken::length() const {
    return m_info.len;
}

const string& LexerToken::filename() const {
    return m_info.filename;
}
