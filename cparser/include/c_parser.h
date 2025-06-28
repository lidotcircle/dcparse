#ifndef _C_PARSER_PARSER_H_
#define _C_PARSER_PARSER_H_

#include "./c_ast.h"
#include "./c_logger.h"
#include "lexer/text_info.h"
#include "parser/parser.h"
#include <set>
#include <string>


namespace cparser {


class CParser;
class CTranslationUnitContext;
;
class CParserContext : public DCParser::DCParserContext, virtual public CLogger
{
  private:
    // Because DCParser is private base class of CParser,
    // DCParser* can not be dynamic_casted to CParser*.
    // This member is used to store the pointer of CParser.
    CParser* m_cparser;
    std::shared_ptr<TextInfo> m_textinfo;
    std::shared_ptr<CTranslationUnitContext> m_tu_context;

  public:
    CParserContext(CParser* cparser);

    inline std::shared_ptr<TextInfo>& textinfo()
    {
        return this->m_textinfo;
    }
    inline CParser* cparser()
    {
        return this->m_cparser;
    }
    inline const CParser* cparser() const
    {
        return this->m_cparser;
    }
    inline std::shared_ptr<CTranslationUnitContext> tu_context()
    {
        return this->m_tu_context;
    }
    inline void set_tu_context(std::shared_ptr<CTranslationUnitContext> tu_context)
    {
        this->m_tu_context = tu_context;
    }
};

class CParser : private DCParser
{
  private:
    void typedef_rule();
    void expression_rules();
    void declaration_rules();
    void statement_rules();
    void external_definitions();

    std::set<std::string> m_typedefs;

    std::shared_ptr<ASTNodeTranslationUnit> get_translation_unit(std::shared_ptr<NonTerminal> node);

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
        return this->get_translation_unit(DCParser::parse(begin, end));
    }

    inline std::shared_ptr<ASTNodeTranslationUnit> parse(ISimpleLexer& lexer)
    {
        return this->get_translation_unit(DCParser::parse(lexer));
    }

    using DCParser::getContext;
    using DCParser::next_possible_token_of;
    using DCParser::prev_possible_token_of;
    using DCParser::query_charinfo;
    using DCParser::reset;
    using DCParser::setDebugStream;
    using DCParser::SetTextinfo;
};

} // namespace cparser
#endif // _C_PARSER_PARSER_H_
