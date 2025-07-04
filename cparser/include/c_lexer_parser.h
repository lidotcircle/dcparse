#ifndef _C_PARSER_LEXER_PARSER_H_
#define _C_PARSER_LEXER_PARSER_H_

#include "./c_ast.h"
#include "./c_parser.h"
#include "./c_token.h"

namespace cparser {

class CLexerParser
{
  private:
    CParser parser;
    CLexerUTF8 lexer;

  public:
    CLexerParser();

    void feed(char c);
    std::shared_ptr<ASTNodeTranslationUnit> end();
    void reset();
    void setDebugStream(std::ostream& os);
};

} // namespace cparser
#endif //_C_PARSER_LEXER_PARSER_H_
