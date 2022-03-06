#ifndef _C_PARSER_PARSER_H_
#define _C_PARSER_PARSER_H_

#include "parser/parser.h"
#include "./c_ast.h"
#include <set>
#include <string>


namespace cparser {


class CParser: private DCParser
{
private:
    void typedef_rule();
    void expression_rules ();
    void declaration_rules();
    void statement_rules  ();
    void external_definitions();

    std::set<std::string> m_typedefs;

    std::shared_ptr<ASTNodeTranslationUnit>
        get_translation_unit(std::shared_ptr<NonTerminal> node);

public:
    CParser();

    using DCParser::feed;

    inline std::shared_ptr<ASTNodeTranslationUnit> end()
    {
        return get_translation_unit(DCParser::end());
    }

    template<typename Iterator>
    std::shared_ptr<ASTNodeTranslationUnit> parse(Iterator begin, Iterator end) 
    {
        return this->get_translation_unit(
                DCParser::parse(begin, end));
    }

    inline std::shared_ptr<ASTNodeTranslationUnit>
        parse(ISimpleLexer& lexer) 
    {
        return this->get_translation_unit(
                DCParser::parse(lexer));
    }

    using DCParser::reset;
    using DCParser::getContext;
    using DCParser::setDebugStream;
    using DCParser::prev_possible_token_of;
    using DCParser::next_possible_token_of;
    using DCParser::query_charinfo;
};

}
#endif // _C_PARSER_PARSER_H_
