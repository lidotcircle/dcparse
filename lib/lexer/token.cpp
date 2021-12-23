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

LexerToken::LexerToken(size_t ln, size_t cn, size_t pos, size_t len, const string& fn):
    m_line(ln), m_column(cn), m_filename(fn), m_pos(pos), m_len(len) {}

size_t LexerToken::line_number() const {
    return m_line;
}

size_t LexerToken::column_number() const {
    return m_column;
}

size_t LexerToken::position() const {
    return m_pos;
}

size_t LexerToken::length() const {
    return m_len;
}

const string& LexerToken::filename() const {
    return m_filename;
}
