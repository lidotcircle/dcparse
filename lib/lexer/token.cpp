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


LexerToken::LexerToken() {}
LexerToken::LexerToken(TextRange range): TextRangeEntity(range) {}

