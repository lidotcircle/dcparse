#include "parser/parser.h"
#include "parser/parser_error.h"
#include "assert.h"
#include <map>
#include <queue>
#include <stdexcept>
using namespace std;

using charid_t = DCParser::charid_t;
using ruleid_t = DCParser::ruleid_t;
using state_t  = DCParser::state_t;
using reduce_callback_t = DCParser::reduce_callback_t;

struct EOFChar: public LexerToken {
    EOFChar(): LexerToken(TokenInfo()) {}
};
charid_t GetEOFChar() { return CharID<EOFChar>(); }

struct RealStartSymbol: public NonTerminal {
    shared_ptr<NonTerminal> sym;

    RealStartSymbol() = delete;
    RealStartSymbol(shared_ptr<NonTerminal> sym): sym(sym) {}
};

struct RuleOption {
    size_t            priority;
    RuleAssocitive    associtive;
};

struct PushdownEntry;
using  PushdownStateLookup = map<state_t, PushdownEntry>;
using  PushdownStateMappingTX = vector<PushdownStateLookup>;
struct PushdownStateMapping {
    PushdownStateMappingTX val; 

    PushdownStateMapping() = delete;

    template<typename T>
    PushdownStateMapping(T v) : val(std::forward<T>(v)) { }
};

struct PushdownEntry
{
public:
    enum PushdownType {
        STATE_TYPE_SHIFT,
        STATE_TYPE_REDUCE,
        STATE_TYPE_LOOKAHEAD,
        STATE_TYPE_REJECT,
    };

private:
    PushdownType _type;
    union MM {
        state_t state;
        ruleid_t rule;
        shared_ptr<PushdownStateLookup> next;

        MM(): state(0) {}
        ~MM() {}
    } _u;

    class StateTypeH {};
    class RuleTypeH {};

    PushdownEntry(state_t state, StateTypeH): _type(STATE_TYPE_SHIFT) { _u.state = state; }
    PushdownEntry(ruleid_t rule, RuleTypeH): _type(STATE_TYPE_REDUCE) { _u.rule = rule; }
    PushdownEntry(shared_ptr<PushdownStateLookup> next)
        : _type(STATE_TYPE_LOOKAHEAD)
    {
        new(&_u.next) shared_ptr<PushdownStateLookup>(next);
    }


public:
    PushdownEntry(): _type(STATE_TYPE_REJECT) { }
    PushdownType type() const { return this->_type; }

    PushdownEntry(const PushdownEntry& other)
        : _type(other._type)
    {
        switch (this->_type) {
        case STATE_TYPE_SHIFT:
            this->_u.state = other._u.state;
            break;
        case STATE_TYPE_REDUCE:
            this->_u.rule = other._u.rule;
            break;
        case STATE_TYPE_LOOKAHEAD:
            new(&this->_u.next) shared_ptr<PushdownStateLookup>(other._u.next);
            break;
        case STATE_TYPE_REJECT:
            break;
        }
    }

    PushdownEntry& operator=(const PushdownEntry& other)
    {
        if (this->type() == STATE_TYPE_LOOKAHEAD) {
            this->_u.next.~shared_ptr<PushdownStateLookup>();
        }

        this->_type = other._type;
        switch (this->_type) {
        case STATE_TYPE_SHIFT:
            this->_u.state = other._u.state;
            break;
        case STATE_TYPE_REDUCE:
            this->_u.rule = other._u.rule;
            break;
        case STATE_TYPE_LOOKAHEAD:
            new(&this->_u.next) shared_ptr<PushdownStateLookup>(other._u.next);
            break;
        case STATE_TYPE_REJECT:
            break;
        }

        return *this;
    }

    state_t state() const {
        assert(this->type() == STATE_TYPE_SHIFT);
        return this->_u.state;
    }

    ruleid_t rule() const {
        assert(this->type() == STATE_TYPE_REDUCE);
        return this->_u.rule;
    }

    const shared_ptr<PushdownStateLookup> lookup() const {
        assert(this->type() == STATE_TYPE_LOOKAHEAD);
        return this->_u.next;
    }

    shared_ptr<PushdownStateLookup> lookup() {
        assert(this->type() == STATE_TYPE_LOOKAHEAD);
        return this->_u.next;
    }

    static PushdownEntry shift(state_t state) { return PushdownEntry(state, StateTypeH()); }
    static PushdownEntry reduce(ruleid_t rule) { return PushdownEntry(rule, RuleTypeH()); }
    static PushdownEntry lookahead(shared_ptr<PushdownStateLookup> next) { return PushdownEntry(next); }
    static PushdownEntry reject() { return PushdownEntry(); }

    ~PushdownEntry()
    {
        if (this->type() == STATE_TYPE_LOOKAHEAD) {
            this->_u.next.~shared_ptr<PushdownStateLookup>();
        }
    }
};

void DCParser::ensure_epsilon_closure()
{
    assert(this->m_real_start_symbol.has_value());

    if (!this->u_epsilon_closure.empty())
        return;

    assert(this->u_possible_next.empty());

    std::map<charid_t,std::set<ruleid_t>> pvpv;
    for (size_t i=0;i<this->m_rules.size();i++) {
        auto& r = this->m_rules[i];
        pvpv[r.m_lhs].insert(i);
    }

    bool changed = true;
    while (changed) {
        changed = false;

        for (auto& rs: pvpv) {
            const auto rx = rs.second;

            for (const auto r: rx) {
                assert(r < this->m_rules.size());
                const auto& rr = this->m_rules[r];

                if (rr.m_rhs.size() > 0) {
                    auto cx = rr.m_rhs[0];

                    if (pvpv.find(cx) != pvpv.end()) {
                        auto& tx = pvpv[cx];
                        const auto old_size = rs.second.size();
                        rs.second.insert(tx.begin(), tx.end());
                        changed = changed || rs.second.size() != old_size;
                    }
                }
            }
        }
    }

    for (auto& rs: pvpv) {
        auto cr = rs.first;
        auto& rx = rs.second;

        for (auto r: rx) {
            this->u_epsilon_closure[cr].push_back(r);
            assert(r < this->m_rules.size());
            auto& rr = this->m_rules[r];

            if (rr.m_rhs.size() > 0) {
                auto cx = rr.m_rhs[0];
                this->u_possible_next[cr].insert(cx);
            }
        }
    }
}

set<pair<ruleid_t,size_t>> DCParser::stateset_move(const set<pair<ruleid_t,size_t>>& old, charid_t ch) const
{
    set<pair<ruleid_t,size_t>> ret;
    auto _this = const_cast<DCParser*>(this);
    _this->ensure_epsilon_closure();

    for (auto& p: old) {
        const auto& r = this->m_rules[p.first];
        const auto pos = p.second;

        assert(pos < r.m_rhs.size() || r.m_rhs.empty());

        if (r.m_rhs[pos] == ch) {
            ret.insert(make_pair(p.first, pos + 1));

            if (pos + 1 < r.m_rhs.size()) {
                auto cx = r.m_rhs[pos + 1];
                if (this->u_epsilon_closure.find(cx) != this->u_epsilon_closure.end()) {
                    for (auto rr: this->u_epsilon_closure.at(cx)) {
                        ret.insert(make_pair(rr, 0));
                    }
                }
            }
        }
    }

    return ret;
}

bool DCParser::is_nonterm(charid_t id) const {
    return this->m_nonterms.find(id) != this->m_nonterms.end();
}

set<pair<ruleid_t,size_t>> DCParser::startState() const {
    assert(this->m_real_start_symbol.has_value());

    set<pair<ruleid_t,size_t>> ret;
    auto _this = const_cast<DCParser*>(this);
    _this->ensure_epsilon_closure();

    assert(!this->m_start_symbols.empty() && "must add at least one start symbol");
    const auto ssym = this->m_real_start_symbol.value();
    assert(this->u_epsilon_closure.find(ssym) != this->u_epsilon_closure.end());
    const auto& start_rules = _this->u_epsilon_closure.at(ssym);
    for (auto& start_rule: start_rules) {
        ret.insert(make_pair(start_rule, 0));
    }

    return ret;
}


DCParser::DCParser(): m_priority(0), m_context(make_unique<DCParserContext>(*this)) { }

void DCParser::dec_priority() { this->m_priority++; }

int DCParser::add_rule_internal(
        charid_t lh, vector<charid_t> rh, vector<bool> rhop,
        reduce_callback_t cb,
        RuleAssocitive associtive)
{
    assert(!this->m_real_start_symbol.has_value());

    this->m_nonterms.insert(lh);
    this->m_symbols.insert(lh);
    if (this->m_terms.find(lh) != this->m_terms.end())
        this->m_terms.erase(lh);

    for (auto r: rh) {
        this->m_symbols.insert(r);
        if (this->m_nonterms.find(r) == this->m_nonterms.end())
            this->m_terms.insert(r);
    }

    size_t ntoken = 0;
    for (auto b: rhop) {
        if (!b)
            ntoken++;
    }
    assert(ntoken == rh.size());

    RuleInfo ri;
    ri.m_lhs = lh;
    ri.m_rhs = rh;
    ri.m_rhs_optional = move(rhop);
    ri.m_reduce_callback = cb;
    ri.m_rule_option = std::make_shared<RuleOption>();

    ri.m_rule_option->associtive = associtive;
    ri.m_rule_option->priority = this->m_priority;

    this->m_rules.push_back(ri);
    return this->m_rules.size() - 1;
}

void DCParser::add_rule(
        charid_t leftside, std::vector<ParserChar> rightside,
        reduce_callback_t reduce_cb, RuleAssocitive associative)
{
    vector<pair<vector<charid_t>,vector<bool>>> rightsides = { { {}, {} } };
    const auto doubleit = [&]() {
        const auto size =rightsides.size();

        for (size_t i=0;i<size;i++)
            rightsides.push_back(rightsides[i]);
    };

    for (auto& rt: rightside) {
        if (rt.optional()) {
            doubleit();
            assert(rightsides.size() % 2 == 0);

            const auto half = rightsides.size() / 2;
            for (size_t i=0;i<half;i++) {
                rightsides[i].first.push_back(rt.cid());
                rightsides[i].second.push_back(false);
            }
            for (size_t i=half;i<rightsides.size();i++) {
                rightsides[i].second.push_back(true);
            }
        } else {
            for (auto& rset: rightsides) {
                rset.first.push_back(rt.cid());
                rset.second.push_back(false);
            }
        }
    }

    for (auto& rset: rightsides)
        this->add_rule_internal(leftside, rset.first, rset.second, reduce_cb, associative);
}


DCParser& DCParser::operator()(charid_t lh, vector<ParserChar> rh,
                               reduce_callback_t cb,
                               RuleAssocitive associtive)
{
    this->add_rule(lh, rh, cb, associtive);
    return *this;
}

void DCParser::add_start_symbol(charid_t id) {
    assert(!this->m_real_start_symbol.has_value());

    if (this->m_start_symbols.find(id) != this->m_start_symbols.end()) {
        throw std::runtime_error("start symbol already exists");
    }

    if (!this->is_nonterm(id)) {
        throw std::runtime_error("start symbol is not a nonterminal");
    }

    this->m_start_symbols.insert(id);
}

void DCParser::setup_real_start_symbol()
{
    assert(!this->m_real_start_symbol.has_value());
    const auto start_sym = CharID<RealStartSymbol>();

    for (auto sym: this->m_start_symbols) {
        this->add_rule( start_sym, { sym }, 
                        [](auto,auto& rn) {
                            assert(rn.size() == 1);
                            auto val = dynamic_pointer_cast<NonTerminal>(rn[0]);
                            assert(val);
                            return make_shared<RealStartSymbol>(val);
                        });
    }

    this->m_real_start_symbol = start_sym;
}

void DCParser::generate_table()
{
    this->setup_real_start_symbol();

    state_t allocated_state = 0;
    map<set<pair<ruleid_t,size_t>>,state_t> state_map;
    const auto query_state = [&](set<pair<ruleid_t,size_t>> s)
    {
        if (state_map.find(s) == state_map.end())
            state_map[s] = allocated_state++;

        return state_map[s];
    };

    const auto s_start_state = this->startState();

    const auto start_state = query_state(s_start_state);
    PushdownStateMappingTX mapping;

    queue<set<pair<ruleid_t,size_t>>> q;
    q.push(s_start_state);
    set<set<pair<ruleid_t,size_t>>> visited( {s_start_state} );

    while(!q.empty()) {
        auto s = q.front();
        q.pop();

        auto state = query_state(s);
        if (mapping.size() <= state)
            mapping.resize(state+1);
        auto& state_mapping = mapping[state];

        for (auto ch: this->m_symbols) {
            auto s_next = this->stateset_move(s, ch);

            // REJECT
            if (s_next.empty()) {
                state_mapping[ch] = PushdownEntry::reject();
                continue;
            }

            set<ruleid_t> completed_rules;
            for (auto& s: s_next) {
                const auto ruleid  = s.first;
                const auto rulepos = s.second;
                assert(this->m_rules.size() > ruleid);
                const auto& rule = this->m_rules[ruleid];

                if (rule.m_rhs.size() == rulepos)
                    completed_rules.insert(ruleid);
            }

            // SHIFT
            if (completed_rules.empty()) {
                auto next_state = query_state(s_next);
                state_mapping[ch] = PushdownEntry::shift(next_state);

                if (visited.find(s_next) == visited.end()) {
                    q.push(s_next);
                    visited.insert(s_next);
                }
                continue;
            }

            // ----- PRIORITY -----
            // 1. IF THERE ARE MULTIPLE RULES WITH HIGHEST PRIORITY ( IN COMPLETED CANDIATE SET ) ARE COMPLETED,
            //    THEN REPORT A GRAMMAR ERROR. THE REASON OF THAT IS IF WE CONTINUE BY LOOKAHEAD, IT'S HIGHLY 
            //    POSSIBLE TO GET AT LEAST ONE REDUCE, WHICH IS AN GRAMMAR ERROR, SO WE SIMPLIFIED THIS PROCESS.
            //    BECAUSE OF THAT IN FOLLOWING ANALYSIS, WE CAN ASSERT THERE IS ONE AND ONLY ONE COMPLETED RULE 
            //    WHICH PRIORITIZE OTHER COMPLETED RULES.
            // 2. IF THE HIGHEST PRIORITY COMPLETED RULE IS HIGHEST PRIORITY (CONSIDER PRIORITY AND ASSOCIATIVE)
            //    IN CANDIDATE LIST, THEN JUST REDUCE
            // 3. OTHERWISE, LOOKAHEAD TABLE IS REQUIRED.


            // ----- ASSOCIATIVE ------
            // a + b + c;  => (a + b) + c
            // LEFT LEFT   => reduce
            //
            // a = b = c   => a = (b = c);
            // RIGHT RIGHT => shift
            //
            // LEFT RIGHT  => ?
            // RIGHT LEFT  => ?
            // shold be resolve be priority, so these two cases are not
            // considered, if which presents raising a grammar error.

            // ----- LOOKAHEAD -----
            // COMPUTE THE SET OF CANDIDATES WHICH PRIORITIZE HIGHEST PRIORITY COMPLETED RULE,
            // THEN FOREACH SYMBOL WE CAN DO A STATE TRANSITION ON THE SET.
            // IF THE STATE SET OF RESULT IS NOT EMPTY THEN SHIFT, OTHERWISE REDUCE.

            vector<ruleid_t> v_completed_candidates(
                    completed_rules.begin(), completed_rules.end());
            std::sort(v_completed_candidates.begin(), v_completed_candidates.end(),
                      [this](ruleid_t a, ruleid_t b) {
                          assert(this->m_rules.size() > a);
                          assert(this->m_rules.size() > b);
                          return this->m_rules[a].m_rule_option->priority <
                                 this->m_rules[b].m_rule_option->priority;
                      });

            const auto completed_highest_priority_rule = v_completed_candidates.front();
            const auto completed_highest_priority = 
                this->m_rules[completed_highest_priority_rule].m_rule_option->priority;
            const auto completed_highest_associtive = 
                this->m_rules[completed_highest_priority_rule].m_rule_option->associtive;

            if (v_completed_candidates.size() > 1) {
                auto  second_pri = v_completed_candidates[1];
                auto& second_pri_rule = this->m_rules[second_pri];

                if (second_pri_rule.m_rule_option->priority == completed_highest_priority) {
                    throw ParserGrammarError("conflict rule");
                }
            }

            set<pair<ruleid_t,size_t>> v_incompleted_candidates;
            for (auto& r: s_next) {
                assert(this->m_rules.size() > r.first);
                auto& rule = this->m_rules[r.first];
                // TODO
                if (rule.m_rhs.size() > r.second && 
                    (rule.m_rule_option->priority < completed_highest_priority || 
                     (rule.m_rule_option->priority == completed_highest_priority &&
                      completed_highest_associtive == RuleAssocitiveRight &&
                      rule.m_rule_option->associtive == RuleAssocitiveRight)))
                {
                    v_incompleted_candidates.insert(r);
                }
            }

            // REDUCE
            if (v_incompleted_candidates.empty()) {
                state_mapping[ch] = PushdownEntry::reduce(completed_highest_priority_rule);
                continue;
            }

            // LOOKAHEAD
            PushdownStateLookup lookahead_table;
            lookahead_table[GetEOFChar()] = PushdownEntry::reduce(completed_highest_priority_rule);
            for (auto& s: this->m_terms) {
                auto s_next = this->stateset_move(v_incompleted_candidates, s);
                
                if (s_next.empty()) {
                    // REDUCE
                    lookahead_table[s] = PushdownEntry::reduce(completed_highest_priority_rule);
                } else {
                    // SHIFT
                    const auto lookahead_shift_state = query_state(v_incompleted_candidates);
                    lookahead_table[s] = PushdownEntry::shift(lookahead_shift_state);
                    if (visited.find(v_incompleted_candidates) == visited.end()) {
                        q.push(v_incompleted_candidates);
                        visited.insert(v_incompleted_candidates);
                    }
                }
            }
            state_mapping[ch] = PushdownEntry::lookahead(std::make_shared<PushdownStateLookup>(move(lookahead_table)));
        }
    }

    assert(mapping.size() == allocated_state);

    this->m_start_state = start_state;
    this->m_pds_mapping = std::make_shared<PushdownStateMapping>(move(mapping));
}

void DCParser::do_shift(state_t state, dchar_t char_)
{
    this->p_state_stack.push_back(state);
    this->p_char_stack.push_back(char_);
}

optional<dchar_t> DCParser::do_reduce(ruleid_t ruleid, dchar_t char_)
{
    assert(!this->p_state_stack.empty());
    assert(this->m_rules.size() > ruleid);

    auto& rule = this->m_rules[ruleid];
    this->p_char_stack.push_back(char_);
    this->p_state_stack.resize(this->p_state_stack.size() + 1);

    auto& rn = rule.m_rhs;
    assert(rn.size() > 0);
    assert(rn.size() <= p_char_stack.size());
    assert(rn.size() <= p_state_stack.size());

    p_state_stack.resize(p_state_stack.size() - rn.size());
    auto ei = p_char_stack.end();
    std::vector<dchar_t> rhs_tokens(ei - rn.size(), ei);
    p_char_stack.resize(p_char_stack.size() - rn.size());

    assert(rn.size() == rhs_tokens.size());
    vector<dchar_t> rhs_tokens_with_optional;
    size_t ni = 0;
    for (auto opt: rule.m_rhs_optional) {
        if (opt) {
            rhs_tokens_with_optional.push_back(nullptr);
        } else {
            rhs_tokens_with_optional.push_back(rhs_tokens[ni++]);
        }
    }
    assert(ni == rhs_tokens.size());

    for (size_t i=0; i<rn.size(); ++i) {
        auto cid = rn[i];
        auto c   = rhs_tokens[i];
        assert(cid == c->charid());
    }

    auto s = rule.m_reduce_callback(*this->m_context, rhs_tokens_with_optional);
    if (s == nullptr)
        throw ParserError("expect a valid token, but get nullptr");

    if (s->charid() != rule.m_lhs)
        throw ParserError("expect a valid token, but get a token with different charid");

    auto nonterm = dynamic_pointer_cast<NonTerminal>(s);
    assert(nonterm != nullptr);

    if (s->charid() == this->m_real_start_symbol.value()) {
        this->p_char_stack.push_back(s);
        return nullopt;
    } else {
        return s;
    }

    return s;
}

optional<dchar_t> DCParser::handle_lookahead(dctoken_t token)
{
    assert(this->p_not_finished.has_value());
    assert(!this->p_state_stack.empty());

    const auto cstate = this->p_state_stack.back();
    auto ptoken = this->p_not_finished.value();
    auto& mapping = this->m_pds_mapping->val;

    assert(mapping.size() > cstate);
    const auto& lookup = mapping[cstate];
    const auto charid = ptoken->charid();
    if (lookup.find(charid) == lookup.end())
        throw ParserUnknownToken("handle_lookahead(): unknown char: " + string(ptoken->charname()));

    const auto& entry = lookup.at(charid);
    assert(entry.type() == PushdownEntry::STATE_TYPE_LOOKAHEAD);

    const auto& state_lookup = entry.lookup();
    if (state_lookup->find(token->charid()) == state_lookup->end())
        throw ParserUnknownToken("handle_lookahead(): unknown lookahead char: " + string(token->charname()));

    const auto& state_entry = state_lookup->at(token->charid());
    assert(state_entry.type() == PushdownEntry::STATE_TYPE_REDUCE ||
           state_entry.type() == PushdownEntry::STATE_TYPE_SHIFT);

    this->p_not_finished = nullopt;
    if (state_entry.type() == PushdownEntry::STATE_TYPE_REDUCE) {
        return this->do_reduce(state_entry.rule(), ptoken);
    } else {
        this->do_shift(state_entry.state(), ptoken);
        return nullopt;
    }
}

void DCParser::feed_internal(dchar_t char_)
{
    assert(!this->p_not_finished.has_value());
    assert(!this->p_state_stack.empty());

    const auto cstate = this->p_state_stack.back();
    auto& mapping = this->m_pds_mapping->val;
    assert(mapping.size() > cstate);
    const auto& lookup = mapping[cstate];
    const auto charid = char_->charid();

    if (lookup.find(charid) == lookup.end())
        throw ParserUnknownToken("feed_internal(): unknown char: " + string(char_->charname()));

    const auto& entry = lookup.at(charid);

    switch (entry.type()) {
        case PushdownEntry::STATE_TYPE_SHIFT:
            this->do_shift(entry.state(), char_);
            break;
        case PushdownEntry::STATE_TYPE_REDUCE: {
            auto nc = this->do_reduce(entry.rule(), char_);
            if (nc.has_value())
                this->feed_internal(nc.value());
         }  break;
        case PushdownEntry::STATE_TYPE_LOOKAHEAD:
            this->p_not_finished = char_;
            break;
        case PushdownEntry::STATE_TYPE_REJECT:
            throw ParserSyntaxError("reject in '" + string(char_->charname()) + "'");
            break;
        default:
            assert(false && "unexpected action type");
    }
}

void DCParser::feed(dctoken_t token)
{
    assert(this->m_pds_mapping);
    assert(this->m_start_state.has_value());

    if (this->p_state_stack.empty())
        this->p_state_stack.push_back(this->m_start_state.value());

    while (this->p_not_finished.has_value()) {
        auto v = this->handle_lookahead(token);

        if (v.has_value())
            this->feed_internal(v.value());
    }

    this->feed_internal(token);
}

dnonterm_t DCParser::end()
{
    assert(!this->p_state_stack.empty());
    assert(!this->p_char_stack.empty() || this->p_not_finished.has_value());

    while (this->p_not_finished.has_value()) {
        auto v = this->handle_lookahead(std::make_shared<EOFChar>());

        if (v.has_value())
            this->feed_internal(v.value());
    }

    assert (!this->p_char_stack.empty());
    if (this->p_char_stack.size() != 1)
        throw ParserSyntaxError("unexpected end of token stream");

    assert(this->p_state_stack.size() >= 1);
    if (this->p_state_stack.size() > 1)
        throw ParserSyntaxError("unexpected end of token stream");

    auto char_ = this->p_char_stack.back();
    auto realstart = dynamic_pointer_cast<RealStartSymbol>(char_);
    assert(realstart && "should be real start symbol");

    return realstart->sym;
}

dnonterm_t DCParser::parse(ISimpleLexer& lexer)
{
    assert(this->m_pds_mapping);
    assert(this->m_start_state.has_value());
    assert(this->p_state_stack.empty());

    while (!lexer.end()) {
        auto token = lexer.next();
        this->feed(token);
    }

    return this->end();
}

void DCParser::reset()
{
    this->p_char_stack.clear();
    this->p_state_stack.clear();
    this->p_not_finished = nullopt;
}
