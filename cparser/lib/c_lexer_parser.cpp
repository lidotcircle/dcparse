#include "c_lexer_parser.h"
using namespace std;
using namespace cparser;


CLexerParser::CLexerParser(): parser(), lexer() {}

void CLexerParser::feed(char c)
{
    auto ts = lexer.feed(c);
    for (auto& t: ts)
        parser.feed(t);
}

shared_ptr<ASTNodeTranslationUnit> CLexerParser::end() 
{
    auto ts = lexer.end();
    for (auto& t: ts)
        parser.feed(t);
    return parser.end();
}

void CLexerParser::reset()
{
    this->lexer.reset();
    this->parser.reset();
}

void CLexerParser::setDebugStream(ostream& os)
{
    this->parser.setDebugStream(os);
}
