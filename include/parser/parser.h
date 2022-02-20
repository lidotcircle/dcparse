#ifndef _DC_PARSER_HPP_
#define _DC_PARSER_HPP_

#include "../lexer/token.h"
#include "../lexer/simple_lexer.hpp"
#include <ostream>
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
        DCParserContext() = delete;
        inline DCParserContext(DCParser& parser): m_parser(&parser) {}

        DCParserContext& operator=(const DCParserContext&) = delete;
        DCParserContext(const DCParserContext&) = delete;

        inline DCParser* parser() { return this->m_parser; }

        virtual ~DCParserContext() = default;
    };

    using charid_t = size_t;
    using ruleid_t = size_t;
    using state_t  = size_t;
    using context_t = std::shared_ptr<DCParserContext>;
    using reduce_callback_t = std::function<dnonterm_t(std::weak_ptr<DCParserContext> context, std::vector<dchar_t>& children)>;

    class ParserChar {
    private:
        DCharInfo _info;
        bool _optional;

    public:
        ParserChar() = delete;
        inline ParserChar(DCharInfo info): _info(info), _optional(false) { }

        inline const char* name() const { return _info.name; }
        inline charid_t cid()     const { return this->_info.id; }
        inline DCharInfo info()   const { return this->_info; }
        inline bool optional()    const { return this->_optional; }

        static inline ParserChar beOptional(DCharInfo info)
        {
            auto c = ParserChar(info);
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
    std::vector<std::set<std::pair<ruleid_t,size_t>>> h_state2set;
    std::map<charid_t,DCharInfo> h_charinfo;
    std::ostream* h_debug_stream;
    void      see_dchar(DCharInfo char_);
    DCharInfo get_dchar(charid_t id) const;
    std::string help_rule2str(ruleid_t rule, size_t pos) const;
    std::string help_when_reject_at(state_t state, charid_t token) const;
    std::string help_print_state(state_t state, size_t count, char paddingchar) const;

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
    // Rules for creating that non-terminal
    std::map<charid_t,std::vector<ruleid_t>> u_nonterm_epsilon_closure;
    // no pratical use
    std::map<charid_t,std::set<charid_t>>    u_nonterm_possible_next;
    void ensure_epsilon_closure();
    std::set<std::pair<ruleid_t,size_t>> stateset_epsilon_closure(const std::set<std::pair<ruleid_t,size_t>>& st);

    bool u_possible_prev_next_computed;
    void compute_posible_prev_next();
    std::map<charid_t,std::set<charid_t>> u_prev_possible_token_of;
    std::map<charid_t,std::set<charid_t>> u_next_possible_token_of;

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

    inline void setContext(context_t context)        { this->m_context = context; }
    inline context_t getContext()             { return this->m_context; }
    inline const context_t getContext() const { return this->m_context; }

    inline void setDebugStream(std::ostream& stream) { this->h_debug_stream = &stream; }

    void add_rule(DCharInfo leftside, std::vector<ParserChar> rightside,
                  reduce_callback_t reduce_cb, RuleAssocitive associative = RuleAssocitiveLeft);

    DCParser& operator()(DCharInfo leftside, std::vector<ParserChar> rightside,
                         reduce_callback_t reduce_cb, RuleAssocitive associative = RuleAssocitiveLeft);

    void add_start_symbol(charid_t start);

    void generate_table();

    std::set<charid_t> prev_possible_token_of(charid_t id) const;
    std::set<charid_t> next_possible_token_of(charid_t id) const;
    DCharInfo query_charinfo(charid_t id) const;

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
