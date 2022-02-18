#ifndef _DC_PARSER_HPP_
#define _DC_PARSER_HPP_

#include "../lexer/token.h"
#include "../lexer/simple_lexer.hpp"
#include <vector>
#include <memory>
#include <tuple>
#include <set>
#include <map>
#include <optional>
#include <functional>

using dchar_t   = std::shared_ptr<DChar>;
using dctoken_t = std::shared_ptr<LexerToken>;

class NonTerminal : public DChar { };
using dnonterm_t = std::shared_ptr<NonTerminal>;

size_t GetEOFChar(); 

struct RuleOption;
enum RuleAssocitive {
    RuleAssocitiveLeft,
    RuleAssocitiveRight,
};
struct PushdownStateMapping;

class DCParser {
public:
    class DCParserContext {
    private:
        DCParser* m_parser;

    public:
        inline DCParserContext(): m_parser(nullptr) {}
        inline DCParserContext(DCParser& parser): m_parser(&parser) {}

        inline DCParser* parser() { return this->m_parser; }
        inline void      set_parser(DCParser& parser) { this->m_parser = &parser; }

        virtual ~DCParserContext() = default;
    };

    using charid_t = size_t;
    using ruleid_t = size_t;
    using state_t  = size_t;
    using context_t = std::unique_ptr<DCParserContext>;
    using reduce_callback_t = std::function<dnonterm_t(DCParserContext& context, std::vector<dchar_t>& children)>;

    class ParserChar {
    private:
        charid_t _cid;
        bool _optional;

    public:
        ParserChar() = delete;
        inline ParserChar(charid_t cid): _cid(cid), _optional(false) { }

        inline charid_t cid() const  { return this->_cid; }
        inline bool optional() const { return this->_optional; }

        static inline ParserChar beOptional(charid_t cid)
        {
            auto c = ParserChar(cid);
            c._optional = true;
            return c; 
        }
        static inline ParserChar beOptional(ParserChar c) { c._optional = true; return c; }
    };


private:
    struct RuleInfo {
        charid_t                    m_lhs;
        std::vector<charid_t>       m_rhs;
        std::vector<bool>           m_rhs_optional;
        reduce_callback_t           m_reduce_callback;
        std::shared_ptr<RuleOption> m_rule_option;
    };
    std::vector<RuleInfo> m_rules;
    context_t m_context;
    std::set<charid_t> m_nonterms;
    std::set<charid_t> m_terms;
    std::set<charid_t> m_symbols;
    std::set<charid_t> m_start_symbols;
    size_t m_priority;

    std::shared_ptr<PushdownStateMapping> m_pds_mapping;
    std::optional<state_t> m_start_state;

    std::vector<state_t> p_state_stack;
    std::vector<dchar_t> p_char_stack;
    std::optional<dchar_t> p_not_finished;

    std::optional<charid_t> m_real_start_symbol;
    void setup_real_start_symbol();

    void                   do_shift(state_t state, dchar_t char_);
    std::optional<dchar_t> do_reduce(ruleid_t rule_id, dchar_t char_);

    std::optional<dchar_t> handle_lookahead(dctoken_t token);
    void feed_internal(dchar_t char_);

private:
    std::map<charid_t,std::vector<ruleid_t>> u_epsilon_closure;
    std::map<charid_t,std::set<charid_t>>    u_possible_next;
    void ensure_epsilon_closure();

    std::set<std::pair<ruleid_t,size_t>>
    stateset_move(const std::set<std::pair<ruleid_t,size_t>>& stateset,
                  charid_t symbol) const;

    bool is_nonterm(charid_t id) const;
    std::set<std::pair<ruleid_t,size_t>> startState() const;

    int  add_rule_internal(charid_t leftside, 
                           std::vector<charid_t> rightside, std::vector<bool> optional,
                           reduce_callback_t reduce_cb, RuleAssocitive associative);

public:
    DCParser();
    virtual ~DCParser() = default;

    void dec_priority();
    inline void __________() { this->dec_priority(); }

    void add_rule(charid_t leftside, std::vector<ParserChar> rightside,
                  reduce_callback_t reduce_cb, RuleAssocitive associative = RuleAssocitiveLeft);

    DCParser& operator()(charid_t leftside, std::vector<ParserChar> rightside,
                         reduce_callback_t reduce_cb, RuleAssocitive associative = RuleAssocitiveLeft);

    void add_start_symbol(charid_t start);

    void generate_table();

    void feed(dctoken_t token);
    dnonterm_t end();

    template<typename Iterator>
    dnonterm_t parse(Iterator begin, Iterator end) {
        assert(m_start_state.has_value());
        assert(this->p_state_stack.empty());

        for (auto it = begin; it != end; ++it) {
            this->feed(*it);
        }
        return this->end();
    }

    dnonterm_t parse(ISimpleLexer& lexer);

    void reset();
};

using ParserChar = DCParser::ParserChar;

#endif // _DC_PARSER_HPP_
