#ifndef _DC_PARSER_REGEX_HPP_
#define _DC_PARSER_REGEX_HPP_

#include <limits>
#include <algorithm>
#include <vector>
#include <memory>
#include <queue>
#include <set>
#include <map>
#include <stdexcept>
#include <string>
#include <sstream>
#include <assert.h>
#include <iostream>
#include "./misc.hpp"


template<typename CharT>
struct character_traits {
    using char_type = CharT;

    constexpr static char_type LPAREN    = '(';
    constexpr static char_type RPAREN    = ')';
    constexpr static char_type LBRACKET  = '[';
    constexpr static char_type CARET     = '^';
    constexpr static char_type DASH      = '-';
    constexpr static char_type RBRACKET  = ']';
    constexpr static char_type OR        = '|';
    constexpr static char_type STAR      = '*';
    constexpr static char_type BACKSLASH = '\\';

    constexpr static char_type LBRACE    = '{';
    constexpr static char_type COMMA     = ',';
    constexpr static char_type RBRACE    = '}';
    constexpr static char_type QUESTION  = '?';
    constexpr static char_type PLUS      = '+';
    constexpr static char_type DOT       = '.';

    constexpr static char_type EMPTY_CHAR = std::numeric_limits<char_type>::min();
    constexpr static char_type MIN = std::numeric_limits<char_type>::min() + 1;
    constexpr static char_type MAX = std::numeric_limits<char_type>::max();
};

template<typename CharT>
std::string char_to_string(CharT c) {
    return std::to_string(c);;
}
template<>
std::string char_to_string<char>(char c);


template<typename Iterator>
struct is_forward_iterator {
    static constexpr bool value = std::is_base_of<
        std::forward_iterator_tag,
        typename std::iterator_traits<Iterator>::iterator_category
    >::value || std::is_same<
        std::forward_iterator_tag,
        typename std::iterator_traits<Iterator>::iterator_category
    >::value;
};
template<typename Iterator, typename valuetype>
struct iterator_value_type_is_same {
    static constexpr bool value = std::is_same<
        typename std::iterator_traits<Iterator>::value_type,
        valuetype
    >::value;
};

template<typename CharT>
class AutomataMatcher {
private:
    using traits = character_traits<CharT>;
    using char_type = CharT;

public:
    virtual void feed(char_type c) = 0;
    virtual bool match() const = 0;
    virtual bool dead() const = 0;
    virtual void reset() = 0;

    template<typename Iterator, typename = typename std::enable_if<
        is_forward_iterator<Iterator>::value &&
        iterator_value_type_is_same<Iterator,char_type>::value,void>::type>
    bool test(Iterator begin, Iterator end) {
        this->reset();
        for (auto it = begin; it != end; ++it)
            feed(*it);
        return match();
    }

    bool test(const std::vector<char_type>& str) {
        return test(str.begin(), str.end());
    }

    bool test(const std::basic_string<char_type>& str) {
        return test(str.begin(), str.end());
    }

    virtual ~AutomataMatcher() = default;
};

template<typename CharT>
class RegexDFA {
public:
    using DFAState_t = size_t;
    using traits = character_traits<CharT>;
    using char_type = CharT;
    struct DFAEntry {
        char_type low, high;
        DFAState_t state;

        DFAEntry(char_type low, char_type high, DFAState_t state)
            : low(low), high(high), state(state) {}
    };
    using DFATransitionTable = std::vector<std::vector<DFAEntry>>;

private:
    DFAState_t m_start_state;
    std::set<DFAState_t> m_dead_states, m_final_states;
    DFATransitionTable m_transitions;

public:
    RegexDFA() = delete;
    RegexDFA(DFATransitionTable table, DFAState_t start_state, std::set<DFAState_t> dead_states, std::set<DFAState_t> final_states):
        m_transitions(std::move(table)), m_start_state(start_state),
        m_dead_states(std::move(dead_states)), m_final_states(std::move(final_states))
    {}

    DFAState_t start_state() const { return m_start_state; }
    const std::set<DFAState_t>& dead_states() const { return m_dead_states; }
    const std::set<DFAState_t>& final_states() const { return m_final_states; }

    DFAState_t state_transition(DFAState_t state, char_type c) const {
        assert(state < m_transitions.size());
        auto& trans = this->m_transitions[state];
        auto lb = std::lower_bound(trans.begin(), trans.end(), c,
            [](const DFAEntry& entry, char_type c) {
                return entry.high < c;
            });
        assert(lb != trans.end());
        assert(lb->low <= c &&  c <= lb->high);
        return lb->state;
    }

    std::string to_string() const {
        std::stringstream ss;
        ss << "start state: " << m_start_state << std::endl;
        ss << "dead states: ";
        for (auto s : m_dead_states)
            ss << s << " ";
        ss << std::endl;
        ss << "final states: ";
        for (auto s : m_final_states)
            ss << s << " ";
        ss << std::endl;
        ss << "transitions: " << std::endl;
        for (size_t i = 0; i < m_transitions.size(); ++i) {
            ss << "state " << i << ": ";
            for (auto& entry : m_transitions[i]) {
                ss << "[" << char_to_string(entry.low) << "-" << char_to_string(entry.high) << "] -> " << entry.state << " ";
            }
            ss << std::endl;
        }
        return ss.str();
    }
};

template<typename CharT>
class DFAMatcher : public AutomataMatcher<CharT> {
private:
    using traits = character_traits<CharT>;
    using char_type = CharT;
    using entry_type= typename RegexDFA<char_type>::DFAEntry;
    using DFAState_t = typename RegexDFA<char_type>::DFAState_t;
    std::shared_ptr<RegexDFA<char_type>> m_dfa;
    DFAState_t m_current_state;

public:
    DFAMatcher() = delete;
    DFAMatcher(std::shared_ptr<RegexDFA<char_type>> dfa):
        m_dfa(dfa)
    {
        if (this->m_dfa == nullptr)
            throw std::runtime_error("DFA is null");

        this->reset();
    }
    DFAMatcher(const std::vector<char_type>& pattern);

    virtual void feed(char_type c) override {
        assert(traits::MIN <= c && c <= traits::MAX);
        auto& dead_states = this->m_dfa->dead_states();
        if (dead_states.find(this->m_current_state) != dead_states.end())
            return;

        auto cstate = this->m_current_state;
        this->m_current_state = this->m_dfa->state_transition(cstate, c);
    }

    virtual bool match() const override {
        auto& finals = m_dfa->final_states();
        return finals.find(this->m_current_state) != finals.end();
    }
    virtual bool dead() const override {
        auto& dead_states = m_dfa->dead_states();
        return dead_states.find(this->m_current_state) != dead_states.end();
    }
    virtual void reset() override {
        this->m_current_state = this->m_dfa->start_state();
    }
    std::string to_string() const {
        return m_dfa->to_string();
    }
    virtual ~DFAMatcher() = default;
};

template<typename CharT>
class RegexNFA {
public:
    using traits = character_traits<CharT>;
    using char_type = CharT;
    using NFAState_t = size_t;
    using DFAState_t = typename RegexDFA<char_type>::DFAState_t;
    struct NFAEntry {
        char_type low, high;
        std::set<NFAState_t> state;

        NFAEntry() = default;
        NFAEntry(char_type low, char_type high): low(low), high(high) {}
        NFAEntry(char_type low, char_type high, std::set<NFAState_t> states):
            low(low), high(high), state(std::move(states))
        {}
    };
    using NFATransitionTable = std::vector<std::vector<NFAEntry>>;

private:
    NFAState_t m_start_state;
    std::set<NFAState_t> m_final_states;
    NFATransitionTable m_transitions;
    std::vector<std::set<NFAState_t>> m_epsilon_closure;
    std::vector<std::vector<std::pair<char_type,char_type>>> m_range_units;

    std::vector<std::set<size_t>> get_epsilon_closure() const
    {
        std::vector<std::set<size_t>> worklist(this->m_transitions.size());
        for (size_t i=0;i<worklist.size();++i) {
            auto& states = this->m_transitions[i];
            if (states.size() > 0 && states[0].low == traits::EMPTY_CHAR) {
                worklist[i].insert(states[0].state.begin(), states[0].state.end());
            }
        }

        return transitive_closure(worklist);
    }

    std::vector<std::vector<std::pair<char_type,char_type>>> range_units_map() const
    {
        assert(this->m_epsilon_closure.size() == this->m_transitions.size());
        std::vector<std::vector<std::pair<char_type,char_type>>> result;
        result.resize(m_transitions.size());

        for (size_t i = 0; i < m_transitions.size(); ++i) {
            auto& ru = result[i];

            for (auto& ci: this->m_epsilon_closure[i]) {
                assert(ci < m_transitions.size());
                auto& trans = this->m_transitions[ci];
                for (auto& m: trans) {
                    if (m.low == traits::EMPTY_CHAR)
                        continue;

                    ru.push_back(std::make_pair(m.low, m.high));
                }
            }
            ru = split_ranges_to_units(std::move(ru));
        }

        return result;
    }

public:
    RegexNFA() = delete;
    RegexNFA(NFATransitionTable table, NFAState_t start_state, std::set<NFAState_t> final_states):
        m_transitions(std::move(table)), m_start_state(start_state), m_final_states(std::move(final_states))
    {
        this->m_epsilon_closure = this->get_epsilon_closure();
        this->m_range_units = this->range_units_map();

        auto& start_closure = m_epsilon_closure[m_start_state];
        if (std::any_of(start_closure.begin(), start_closure.end(), [this](size_t i) {
            return this->m_final_states.find(i) != this->m_final_states.end();
        }))
        {
            m_final_states.insert(m_start_state);
        }
    }

    NFAState_t start_state() const { return m_start_state; }
    const std::set<NFAState_t>& final_states() const { return m_final_states; }
    const std::vector<std::set<NFAState_t>>& epsilon_closure() const { return m_epsilon_closure; }

    std::set<NFAState_t> state_transition(std::set<NFAState_t> stateset, char_type c) const
    {
        auto& nfa_epsilon_closure = this->epsilon_closure();
        std::set<NFAState_t> next_states;
        for (auto& state : stateset) {
            auto& transitions = this->m_transitions;
            assert(transitions.size() > state);
            auto& transition = transitions[state];
            auto lb = std::lower_bound(transition.begin(), transition.end(), c,
                [](const NFAEntry& entry, char_type c) {
                    return entry.high < c;
                });

            if (lb != transition.end() && lb->low <= c && c <= lb->high) {
                for (auto k=lb->state.begin(); k!=lb->state.end(); ++k) {
                    assert(*k < nfa_epsilon_closure.size());
                    const auto& cl = nfa_epsilon_closure[*k];
                    next_states.insert(cl.begin(), cl.end());
                }
            }
        }

        return next_states;
    }

    std::string to_string() const {
        std::ostringstream ss;
        ss << "start: " << m_start_state << std::endl;
        ss << "final: ";
        for (auto& s: m_final_states)
            ss << s << " ";
        ss << std::endl;
        ss << "transitions: " << std::endl;
        for (size_t i = 0; i < m_transitions.size(); ++i) {
            ss << i << ": ";
            for (auto& e: m_transitions[i]) {
                if (e.low == traits::EMPTY_CHAR) {
                    ss << "ε -> {";
                } else if (e.low == e.high) {
                    ss << char_to_string(e.low) << " -> {";
                } else {
                    ss << "[" << char_to_string(e.low) << "-" << char_to_string(e.high) << "] -> {";
                }
                for (auto& s: e.state)
                    ss << s << " ";
                ss << "} ";
            }
            ss << std::endl;
        }
        return ss.str();
    }

    RegexDFA<char_type> compile() const
    {
        size_t dfa_state_count = 0;
        std::map<std::set<NFAState_t>,DFAState_t> dfa_state_map;
        const auto query_dfa_state = [&](std::set<NFAState_t> states) -> DFAState_t {
            auto it = dfa_state_map.find(states);
            if (it != dfa_state_map.end())
                return it->second;

            const auto val = dfa_state_count++;
            dfa_state_map[states] = val;
            return val;
        };

        typename RegexDFA<char_type>::DFATransitionTable transtable;
        const auto start_state = query_dfa_state({this->m_start_state});
        const auto dead_state = query_dfa_state({ });
        transtable.resize(dfa_state_count);
        transtable[dead_state].emplace_back(traits::MIN, traits::MAX, dead_state);

        const auto state_trans = [&](NFAState_t state, std::pair<char_type,char_type> ch) {
            assert(state < this->m_transitions.size());
            auto& trans = this->m_transitions[state];
            auto lb = std::lower_bound(trans.begin(), trans.end(), ch.second,
                                       [](const auto& trans, char_type bd) { return trans.high < bd; });
            if (lb == trans.end())
                return std::set<size_t>();
            
            assert(lb->low >= ch.first);
            return lb->state;
        };

        const auto state_set_trans = [&](const std::set<NFAState_t>& states, std::pair<char_type,char_type> ch)
        {
            std::set<NFAState_t> next_state;
            for (auto& s: states) {
                auto sn = state_trans(s, ch);
                for (auto& ks: sn) {
                    auto& mm = this->m_epsilon_closure[ks];
                    next_state.insert(mm.begin(), mm.end());
                }
            }

            return next_state;
        };

        std::queue<std::set<NFAState_t>> process_queue;
        process_queue.push({ this->m_start_state });
        std::set<std::set<NFAState_t>> processed_states={ { this->m_start_state } };

        while (!process_queue.empty()) {
            auto stateset = process_queue.front();
            process_queue.pop();
            auto state = query_dfa_state(stateset);
            if (transtable.size() <= state)
                transtable.resize(state + 1);
            auto& trans = transtable[state];

            std::set<std::pair<char_type,char_type>> range_units;
            for (auto& s: stateset) {
                assert(m_range_units.size() > s);
                auto& range_unit = m_range_units[s];
                range_units.insert(range_unit.begin(), range_unit.end());
            }
            auto ranges = split_ranges_to_units(
                    std::vector<std::pair<char_type,char_type>>(range_units.begin(), range_units.end()));

            auto lv = traits::MIN;
            for (auto& r: ranges) {
                auto sn = state_set_trans(stateset, r);
                if (processed_states.find(sn) == processed_states.end()) {
                    process_queue.push(sn);
                    processed_states.insert(sn);
                }

                auto n = query_dfa_state(sn);
                if (lv == traits::MAX) {
                    assert(r.first == r.second);
                    assert(r.first == traits::MAX);
                }

                if (r.first > lv)
                    trans.emplace_back(lv, r.first - 1, dead_state);
                trans.emplace_back(r.first, r.second, n);
                if (r.second < traits::MAX) {
                    lv = r.second + 1;
                } else {
                    assert(r.second == traits::MAX);
                    lv = traits::MAX;
                }
            }

            if (trans.empty()) {
                trans.emplace_back(traits::MIN, traits::MAX, dead_state);
            } else if (trans.back().high < traits::MAX) {
                trans.emplace_back(trans.back().high + 1, traits::MAX, dead_state);
            }
        }

        std::set<DFAState_t> final_states;
        for (auto& s: processed_states) {
            for (auto& f: this->m_final_states) {
                if (s.find(f) != s.end()) {
                    final_states.insert(query_dfa_state(s));
                    break;
                }
            }
        }

        return RegexDFA<char_type>(transtable, start_state, { dead_state }, final_states);
    }
};

template<typename CharT>
class NFAMatcher : public AutomataMatcher<CharT> {
private:
    using traits = character_traits<CharT>;
    using char_type = CharT;
    using NFAEntry = typename RegexNFA<char_type>::NFAEntry;
    using NFAState_t = typename RegexNFA<char_type>::NFAState_t;
    std::shared_ptr<RegexNFA<char_type>> m_nfa;
    std::set<NFAState_t> m_current_states;

public:
    NFAMatcher() = delete;
    NFAMatcher(std::shared_ptr<RegexNFA<char_type>> nfa):
        m_nfa(nfa)
    {
        if (this->m_nfa == nullptr)
            throw std::runtime_error("NFA is null");

        this->reset();
    }
    NFAMatcher(const std::vector<char_type>& pattern);

    std::shared_ptr<RegexNFA<char_type>> get_nfa() { return this->m_nfa; }

    virtual void feed(char_type c) override {
        assert(traits::MIN <= c && c <= traits::MAX);
        if (this->m_current_states.empty())
            return;

        auto next = this->m_nfa->state_transition(this->m_current_states, c);
        this->m_current_states = next;
    }
    virtual bool match() const override {
        auto& finals = m_nfa->final_states();
        return std::any_of(m_current_states.begin(), m_current_states.end(),
            [&finals](NFAState_t state) {
                return finals.find(state) != finals.end();
            });
    }
    virtual bool dead() const override {
        /** by inductive reasoning (for concatenation, union and kleene star)
         *  we can prove any state can reach final state */
        return m_current_states.empty();
    }
    virtual void reset() override {
        m_current_states = { m_nfa->start_state() };
    }
    std::string to_string() const { return m_nfa->to_string(); }
    virtual ~NFAMatcher() = default;
};

template<typename CharT>
class Regex2BasicConvertor {
private:
    using traits = character_traits<CharT>;
    using char_type = CharT;
    std::vector<size_t> m_lparen_pos;
    std::vector<char_type> __result;
    bool escaping;
    bool endding;
    size_t m_last_lparen_pos;
    static constexpr size_t npos = std::numeric_limits<size_t>::max();

    bool m_in_bracket_mode;
    bool m_escaping_in_bracket_mode;

    bool m_in_brace_mode;
    bool m_brace_got_comma;
    int  m_brace_count_low;
    int  m_brace_count_up;
    static constexpr size_t brace_infinity = std::numeric_limits<decltype(m_brace_count_up)>::max();
    std::vector<char_type> __copy_content;

    std::vector<char_type> get_last_node(size_t last_lparen_pos)
    {
        if (this->__result.empty())
            throw std::runtime_error("unexpected {} / + / ?");

        if (last_lparen_pos == npos) {
            const auto __rsize = __result.size();
            if (__rsize >= 2 && 
                this->__result[__rsize - 1] == traits::BACKSLASH &&
                this->__result[__rsize - 2] == traits::BACKSLASH)
            {
                return std::vector<char_type>({  traits::BACKSLASH, traits::BACKSLASH });
            } else {
                return std::vector<char_type>({  this->__result.back() });
            }
        } else {
            assert(this->__result.back() == traits::RPAREN);
            std::vector<char_type> ans;
            std::copy(this->__result.begin() + last_lparen_pos, this->__result.end(), std::back_inserter(ans));
            return ans;
        }
    }

    bool handle_bracket_mode(char_type c) {
        if (!this->m_in_bracket_mode)
            return false;

        this->__result.push_back(c);

        if (c == traits::BACKSLASH) {
            this->m_escaping_in_bracket_mode = !this->m_escaping_in_bracket_mode;
        } else {
            this->m_escaping_in_bracket_mode = false;
            this->m_in_bracket_mode = c != traits::RBRACKET;
        }

        return true;
    }

    void enter_brace_mode() {
        this->m_brace_count_low = 0;
        this->m_brace_count_up = 0;
        this->m_in_brace_mode = true;
        this->m_brace_got_comma = false;
    }

    bool handle_brace_mode(char_type c) {
        if (!this->m_in_brace_mode)
            return false;

        if (c == traits::COMMA) {
            if (this->m_brace_got_comma)
                throw std::runtime_error("unexpected , in brace");
            this->m_brace_got_comma = true;
            this->m_brace_count_up=brace_infinity;
            return true;
        } else if (c == traits::RBRACE) {
            this->__result.resize(this->__result.size() - this->__copy_content.size());
            for (size_t i=0; i<this->m_brace_count_low; i++) {
                this->__result.insert(this->__result.end(), this->__copy_content.begin(), this->__copy_content.end());
            }

            if (this->m_brace_got_comma) {
                if (this->m_brace_count_up < this->m_brace_count_low)
                    throw std::runtime_error("range error");

                if (this->m_brace_count_up == brace_infinity) {
                    this->__result.insert(this->__result.end(), this->__copy_content.begin(), this->__copy_content.end());
                    this->__result.push_back(traits::STAR);
                } else {
                    const auto opt_count = this->m_brace_count_up - this->m_brace_count_low;
                    for (size_t i=0; i<opt_count; i++) {
                        this->__result.push_back(traits::LPAREN);
                        this->__result.push_back(traits::OR);
                        this->__result.insert(this->__result.end(), this->__copy_content.begin(), this->__copy_content.end());
                    }
                    std::vector<char_type> nrparen(opt_count, traits::RPAREN);
                    this->__result.insert(this->__result.end(), nrparen.begin(), nrparen.end());
                }
            }

            this->m_in_brace_mode = false;
            return true;
        }

        if (c < '0' || c > '9')
            throw std::runtime_error("brace mode: expect number, bug get '" + char_to_string(c) + "'");

        if (this->m_brace_got_comma) {
            if (this->m_brace_count_up == brace_infinity)
                this->m_brace_count_up = 0;

            this->m_brace_count_up *= 10;
            this->m_brace_count_up += c - '0';
        } else {
            this->m_brace_count_low *= 10;
            this->m_brace_count_low += c - '0';
        }

        return true;
    }

public:
    Regex2BasicConvertor():
        escaping(false), endding(false),
        m_last_lparen_pos(npos),
        m_in_bracket_mode(false), m_in_brace_mode(false) {}

    void feed(char_type c) {
        assert(!this->endding);
        auto last_lparen_pos = this->m_last_lparen_pos;
        this->m_last_lparen_pos = npos;

        if (this->handle_bracket_mode(c))
            return;

        if (this->handle_brace_mode(c))
            return;

        if (this->escaping) {
            switch (c) {
                case traits::LBRACE:
                case traits::RBRACE:
                case traits::QUESTION:
                case traits::PLUS:
                case traits::DOT:
                    this->__result.push_back(c);
                    break;
                default:
                    this->__result.push_back(traits::BACKSLASH);
                    this->__result.push_back(c);
                    break;
            }

            this->escaping = false;
            return;
        }

        switch (c) {
            case traits::LPAREN:
                this->m_lparen_pos.push_back(this->__result.size());
                this->__result.push_back(c);
                break;
            case traits::RPAREN:
                if (this->m_lparen_pos.empty())
                    throw std::runtime_error("unmatched right parenthese");
                this->__result.push_back(c);
                this->m_last_lparen_pos = this->m_lparen_pos.back();
                this->m_lparen_pos.pop_back();
                break;
            case traits::BACKSLASH:
                this->escaping = true;
                break;
            case traits::LBRACE:
                this->enter_brace_mode();
                this->__copy_content = this->get_last_node(last_lparen_pos);
                break;
            case traits::RBRACE:
                throw std::runtime_error("unexpected rbrace");
                break;
            case traits::LBRACKET:
                this->m_in_bracket_mode = true;
                this->m_escaping_in_bracket_mode = false;
                this->__result.push_back(c);
                break;
            case traits::QUESTION: {
                auto last_node = this->get_last_node(last_lparen_pos);
                assert(this->__result.size() >= last_node.size());
                this->__result.resize(this->__result.size() - last_node.size());
                this->__result.push_back(traits::LPAREN);
                this->__result.push_back(traits::OR);
                this->__result.insert(this->__result.end(), last_node.begin(), last_node.end());
                this->__result.push_back(traits::RPAREN);
            } break;
            case traits::PLUS: {
                auto last_node = this->get_last_node(last_lparen_pos);
                this->__result.insert(this->__result.end(), last_node.begin(), last_node.end());
                this->__result.push_back(traits::STAR);
            } break;
            case traits::DOT:
                this->__result.push_back(traits::LBRACKET);
                this->__result.push_back(traits::MIN);
                this->__result.push_back(traits::DASH);
                this->__result.push_back(traits::MAX);
                this->__result.push_back(traits::RBRACKET);
                break;
            default:
                this->__result.push_back(c);
                break;
        }
    }

    std::vector<char_type>& end() {
        assert(!this->endding);
        this->endding = true;

        if (this->escaping)
            throw std::runtime_error("Regex: unclosed escape");

        return __result;
    }

    static std::vector<char_type> convert(const std::vector<char_type>& regex) {
        Regex2BasicConvertor conv;
        for (auto c : regex)
            conv.feed(c);
        return conv.end();
    }
};

template<size_t Begin = 0, size_t End = std::numeric_limits<size_t>::max()>
class StateAllocator {
private:
    static_assert(Begin < End, "Begin must be less than End");
    size_t last_state;

public:
    StateAllocator() : last_state(Begin) {}

    size_t newstate() {
        if (last_state == End)
            throw std::runtime_error("Too many states");

        return last_state++;
    }
};

template<typename CharT>
class NodeNFA {
public:
    using traits = character_traits<CharT>;
    using char_type = CharT;
    using NodeNFAEntry = typename RegexNFA<char_type>::NFAEntry;
    using NFAState_t = typename RegexNFA<char_type>::NFAState_t;
    using NodeNFATransitionTable = std::map<size_t,std::vector<NodeNFAEntry>>;

private:
    NFAState_t m_start_state, m_final_state;
    NodeNFATransitionTable m_transitions;

    static std::vector<NodeNFAEntry> merge_two_transitions(
            std::vector<NodeNFAEntry> first,
            std::vector<NodeNFAEntry> second)
    {
        std::vector<NodeNFAEntry> result;
        result.reserve(first.size() + second.size());

        auto first_it = first.begin();
        auto second_it = second.begin();
        while (first_it != first.end() && second_it != second.end()) {
            if (first_it->high < second_it->low) {
                result.push_back(std::move(*first_it));
                ++first_it;
            } else if (first_it->low > second_it->high) {
                result.push_back(std::move(*second_it));
                ++second_it;
            } else {
                auto& sm = first_it->low <  second_it->low ? first_it : second_it;
                auto& bg = first_it->low >= second_it->low ? first_it : second_it;
                
                if (sm->low < bg->low) {
                    result.push_back(NodeNFAEntry(sm->low, (char_type)(bg->low - 1), sm->state));
                    sm->low = bg->low;
                    assert(sm->low <= sm->high);
                }

                auto& sm2 = sm->high <  bg->high ? sm : bg;
                auto& bg2 = sm->high >= bg->high ? sm : bg;
                result.push_back(NodeNFAEntry(sm2->low, sm2->high, std::move(sm2->state)));
                result.back().state.insert(bg2->state.begin(), bg2->state.end());
                sm2++;

                if (sm2->high + 1 <= bg2->high) {
                    bg2->low = (char_type)(sm->high + 1);
                } else {
                    bg2++;
                }
            }
        }

        if (first_it != first.end()) {
            for (; first_it != first.end(); ++first_it)
                result.push_back(std::move(*first_it));
        }
        if (second_it != second.end()) {
            for (; second_it != second.end(); ++second_it)
                result.push_back(std::move(*second_it));
        }

        return result;
    }


public:
    NodeNFA() = delete;
    NodeNFA(NodeNFATransitionTable transitions, NFAState_t start_state, NFAState_t final_state) :
        m_transitions(std::move(transitions)), m_start_state(start_state), m_final_state(final_state) {}

    NodeNFATransitionTable& transitions() { return m_transitions; }
    const NodeNFATransitionTable& transitions() const { return m_transitions; }
    NFAState_t start_state() { return m_start_state; }
    NFAState_t final_state() { return m_final_state; }

    RegexNFA<char_type> toRegexNFA()
    {
        typename RegexNFA<char_type>::NFATransitionTable transitions;
        transitions.resize(this->m_transitions.size());
        assert(transitions.size() == this->m_transitions.size());

        std::map<size_t,size_t> state_rewriter;
        size_t state_size = 0;
        const auto query_state = [&](size_t state) {
            if (state_rewriter.find(state) == state_rewriter.end())
                state_rewriter[state] = state_size++;
            return state_rewriter[state];
        };
        const auto query_state_set = [&](std::set<size_t> states) {
            std::set<size_t> result;
            for (auto state : states)
                result.insert(query_state(state));
            return result;
        };

        for (auto& m: this->m_transitions) {
            auto state = query_state(m.first);
            if (transitions.size() < state_size)
                transitions.resize(state_size);
            assert(state < transitions.size());

            auto& entries = m.second;
            auto& new_entries = transitions[state];
            for (auto& entry : entries) {
                auto& new_entry = new_entries.emplace_back();
                new_entry.state = query_state_set(entry.state);
                new_entry.low = entry.low;
                new_entry.high = entry.high;
            }
        }

        transitions.resize(state_size);
        auto start_state = query_state(this->m_start_state);
        auto finals = query_state(m_final_state);
        return RegexNFA<char_type>(transitions, start_state, { finals });
    }

    std::string to_string() const
    {
        std::ostringstream ss;
        ss << "start: " << m_start_state << std::endl;
        ss << "final: " << m_final_state << std::endl;
        ss << "transitions: " << std::endl;
        for (auto& trans: this->m_transitions) {
            ss << trans.first << ": ";
            for (auto& e: trans.second) {
                if (e.low == traits::EMPTY_CHAR) {
                    ss << "ε -> {";
                } else if (e.low == e.high) {
                    ss << char_to_string(e.low) << " -> {";
                } else {
                    ss << "[" << char_to_string(e.low) << "-" << char_to_string(e.high) << "] -> {";
                }
                for (auto& s: e.state)
                    ss << s << " ";
                ss << "} ";
            }
            ss << std::endl;
        }
        return ss.str();
    }

    void merge_with(NFAState_t state, std::vector<NodeNFAEntry> entries)
    {
        auto& transitions = this->m_transitions;

        if (transitions.find(state) == transitions.end()) {
            transitions[state] = std::move(entries);
        } else {
            transitions[state] = NodeNFA<char_type>::merge_two_transitions(
                    std::move(entries),
                    std::move(transitions[state]));
        }
    }

    void add_link(NFAState_t from, std::set<NFAState_t> to, char_type low, char_type high)
    {
        this->merge_with(from, { NodeNFAEntry(low, high, to) });
    }

    static NodeNFA<char_type> from_basic_regex(const std::vector<char_type>& regex);
    static NodeNFA<char_type> from_regex(const std::vector<char_type>& regex) {
        Regex2BasicConvertor<char_type> conv;
        auto basic_regex = conv.convert(regex);
        return from_basic_regex(basic_regex);
    }
};

enum ExprNodeType {
    ExprNodeType_Empty, ExprNodeType_Group, ExprNodeType_CharRange,
    ExprNodeType_Concatenation, ExprNodeType_Union,  ExprNodeType_KleeneStar
};

template<typename CharT>
class ExprNode {
public:
    using NFAState_t = typename NodeNFA<CharT>::NFAState_t;
    virtual ExprNodeType node_type() const = 0;
    virtual std::string to_string() const = 0;

    virtual NodeNFA<CharT> to_nfa(StateAllocator<>& allocator, NFAState_t starts, NFAState_t finals) const = 0;

    virtual ~ExprNode() = default;
};

template<typename CharT>
class ExprNodeEmpty : public ExprNode<CharT> {
private:
    using traits = character_traits<CharT>;
    using char_type = CharT;
    using NodeNFATransitionTable = typename NodeNFA<CharT>::NodeNFATransitionTable;
    using NodeNFAEntry = typename NodeNFA<CharT>::NodeNFAEntry;
    using NFAState_t = typename NodeNFA<CharT>::NFAState_t;

public:
    virtual ExprNodeType node_type() const override { return ExprNodeType_Empty; }
    virtual std::string to_string() const override {
        return "";
    }

    virtual NodeNFA<CharT> to_nfa(StateAllocator<>& allocator, NFAState_t starts, NFAState_t finals) const
    {
        NodeNFA<CharT> retval(NodeNFATransitionTable(), starts, finals);;
        retval.add_link(starts, { finals }, traits::EMPTY_CHAR, traits::EMPTY_CHAR);
        return retval;
    }

    virtual ~ExprNodeEmpty() override = default;
};

template<typename CharT>
class ExprNodeGroup : public ExprNode<CharT> {
private:
    using NFAState_t = typename NodeNFA<CharT>::NFAState_t;
    std::shared_ptr<ExprNode<CharT>> _next;

public:
    ExprNodeGroup(std::shared_ptr<ExprNode<CharT>> next) : _next(next) {}

    const std::shared_ptr<ExprNode<CharT>> next() const { return _next; }
    std::shared_ptr<ExprNode<CharT>> next() { return _next; }

    virtual ExprNodeType node_type() const override { return ExprNodeType_Group; }
    virtual std::string to_string() const override {
        return "(" + this->_next->to_string() + ")";
    }

    virtual NodeNFA<CharT> to_nfa(StateAllocator<>& allocator, NFAState_t starts, NFAState_t finals) const
    {
        return this->_next->to_nfa(allocator, starts, finals);
    }

    virtual ~ExprNodeGroup() override = default;
};

template<typename CharT>
class ExprNodeCharRange : public ExprNode<CharT> {
private:
    using traits = character_traits<CharT>;
    using char_type = CharT;
    using NodeNFATransitionTable = typename NodeNFA<CharT>::NodeNFATransitionTable;
    using NodeNFAEntry = typename NodeNFA<CharT>::NodeNFAEntry;
    using NFAState_t = typename NodeNFA<CharT>::NFAState_t;
    char_type _min, _max;

public:
    ExprNodeCharRange(char_type min, char_type max) : _min(min), _max(max)
    {
        assert(min <= max);
        assert(traits::MIN <= min && min <= traits::MAX);
        assert(traits::MIN <= max && max <= traits::MAX);
    }

    char_type min() const { return _min; }
    char_type max() const { return _max; }

    virtual ExprNodeType node_type() const override { return ExprNodeType_CharRange; }
    virtual std::string to_string() const override {
        if (this->_min == this->_max)
            return char_to_string(this->_min);
        else
            return char_to_string(this->_min) + "-" + char_to_string(this->_max);
    }

    virtual NodeNFA<CharT> to_nfa(StateAllocator<>& allocator, NFAState_t starts, NFAState_t finals) const
    {
        NodeNFA<CharT> retval(NodeNFATransitionTable(), starts, finals);
        retval.add_link(starts, { finals }, this->min(), this->max());
        return retval;
    }

    virtual ~ExprNodeCharRange() override = default;
};

template<typename CharT>
class ExprNodeConcatenation : public ExprNode<CharT> {
private:
    using traits = character_traits<CharT>;
    using char_type = CharT;
    using NodeNFATransitionTable = typename NodeNFA<CharT>::NodeNFATransitionTable;
    using NodeNFAEntry = typename NodeNFA<CharT>::NodeNFAEntry;
    using NFAState_t = typename NodeNFA<CharT>::NFAState_t;
    std::vector<std::shared_ptr<ExprNode<CharT>>> children;

public:
    ExprNodeConcatenation() {}

    const std::vector<std::shared_ptr<ExprNode<CharT>>>& children_ref() const { return children; }
    const size_t size() const { return children.size(); }

    void add_child(std::shared_ptr<ExprNode<CharT>> child) { children.push_back(child); }
    auto& back() { return children.back(); }
    const auto& back() const { return children.back(); }

    virtual ExprNodeType node_type() const override { return ExprNodeType_Concatenation; }
    virtual std::string to_string() const override {
        std::string result;
        for (auto& child : children)
            result += child->to_string();
        return result;
    }

    virtual NodeNFA<CharT> to_nfa(StateAllocator<>& allocator, NFAState_t starts, NFAState_t finals) const
    {
        assert(this->size() > 0);
        if (this->size() == 1)
            return this->back()->to_nfa(allocator, starts, finals);

        NodeNFA<CharT> retval(NodeNFATransitionTable(), starts, finals);

        std::vector<std::pair<size_t,size_t>> starts_finals(this->size());
        starts_finals.front().first = starts;
        starts_finals.back().second = finals;
        for (size_t i=0;i<starts_finals.size()-1;i++) {
            auto ns = allocator.newstate();
            starts_finals[i].second = ns;
            starts_finals[i+1].first = ns;
        }

        for (size_t i=0;i<starts_finals.size();i++) {
            auto s = starts_finals[i].first;
            auto f = starts_finals[i].second;
            auto& child = this->children[i];
            auto child_nfa = child->to_nfa(allocator, s, f);
            auto& child_transitions = child_nfa.transitions();
            for (auto& entryp : child_transitions) {
                auto& w = entryp.first;
                auto& entry = entryp.second;

                retval.merge_with(w, entry);
            }
        }

        return retval;
    }

    virtual ~ExprNodeConcatenation() override = default;
};

template<typename CharT>
class ExprNodeUnion : public ExprNode<CharT> {
private:
    using traits = character_traits<CharT>;
    using char_type = CharT;
    using NodeNFATransitionTable = typename NodeNFA<CharT>::NodeNFATransitionTable;
    using NodeNFAEntry = typename NodeNFA<CharT>::NodeNFAEntry;
    using NFAState_t = typename NodeNFA<CharT>::NFAState_t;
    std::vector<std::shared_ptr<ExprNode<CharT>>> children;

public:
    ExprNodeUnion() {}

    const std::vector<std::shared_ptr<ExprNode<CharT>>>& children_ref() const { return children; }
    const size_t size() const { return children.size(); }

    void add_child(std::shared_ptr<ExprNode<CharT>> child) { children.push_back(child); }
    auto& back() { return children.back(); }
    const auto& back() const { return children.back(); }

    virtual ExprNodeType node_type() const override { return ExprNodeType_Union; }
    virtual std::string to_string() const override {
        std::string result;
        for (auto& child : children)
            result += child->to_string() + "|";
        result.erase(result.end() - 1);
        return result;
    }

    virtual NodeNFA<CharT> to_nfa(StateAllocator<>& allocator, NFAState_t starts, NFAState_t finals) const
    {
        assert(this->size() > 0);
        if (this->size() == 1)
            return this->back()->to_nfa(allocator, starts, finals);

        NodeNFA<CharT> retval(NodeNFATransitionTable(), starts, finals);

        for (auto& child: this->children) {
            auto child_nfa = child->to_nfa(allocator, starts, finals);
            auto& child_transitions = child_nfa.transitions();
            for (auto& entryp : child_transitions) {
                auto& w = entryp.first;
                auto& entry = entryp.second;

                retval.merge_with(w, entry);
            }
        }

        return retval;
    }

    virtual ~ExprNodeUnion() override = default;
};

template<typename CharT>
class ExprNodeKleeneStar : public ExprNode<CharT> {
private:
    using traits = character_traits<CharT>;
    using char_type = CharT;
    using NodeNFATransitionTable = typename NodeNFA<CharT>::NodeNFATransitionTable;
    using NodeNFAEntry = typename NodeNFA<CharT>::NodeNFAEntry;
    using NFAState_t = typename NodeNFA<CharT>::NFAState_t;
    std::shared_ptr<ExprNode<CharT>> _next;

public:
    ExprNodeKleeneStar(std::shared_ptr<ExprNode<CharT>> next) : _next(next) {}

    const std::shared_ptr<ExprNode<CharT>> next() const { return _next; }
    std::shared_ptr<ExprNode<CharT>> next() { return _next; }

    virtual ExprNodeType node_type() const override { return ExprNodeType_KleeneStar; }
    virtual std::string to_string() const override {
        return this->_next->to_string() + "*";
    }

    virtual NodeNFA<CharT> to_nfa(StateAllocator<>& allocator, NFAState_t starts, NFAState_t finals) const
    {
        auto retval = this->next()->to_nfa(allocator, starts, finals);
        retval.add_link(starts, { finals }, traits::EMPTY_CHAR, traits::EMPTY_CHAR);
        retval.add_link(finals, { starts }, traits::EMPTY_CHAR, traits::EMPTY_CHAR);
        return retval;
    }

    virtual ~ExprNodeKleeneStar() override = default;
};

template<typename CharT>
class RegexNodeTreeGenerator {
private:
    using traits = character_traits<CharT>;
    using char_type = CharT;
    using node_type = std::shared_ptr<ExprNode<char_type>>;;
    struct StackValueState {
        std::shared_ptr<ExprNode<char_type>> node;

        StackValueState(): node(std::make_shared<ExprNodeEmpty<char_type>>()) {}
    };
    std::vector<StackValueState> _stack;
    bool _endding;
    bool _escaping;

    node_type push_node_to_node(node_type oldnode, node_type node)
    {
        assert(oldnode);
        node_type ret;

        switch (oldnode->node_type())
        {
            case ExprNodeType_Concatenation: {
                auto ptr = std::dynamic_pointer_cast<ExprNodeConcatenation<char_type>>(oldnode);
                ptr->add_child(node);
                ret = oldnode;
            } break;
            case ExprNodeType_Union: {
                auto ptr = std::dynamic_pointer_cast<ExprNodeUnion<char_type>>(oldnode);
                auto& last = ptr->back();
                last = push_node_to_node(last, node);
                ret = oldnode;
            } break;
            case ExprNodeType_Group:
            case ExprNodeType_CharRange:
            case ExprNodeType_KleeneStar: {
                auto newnode = std::make_shared<ExprNodeConcatenation<char_type>>();
                newnode->add_child(oldnode);
                newnode->add_child(node);
                ret = newnode;
            } break;
            case ExprNodeType_Empty: {
                ret = node;
            } break;
            default:
                assert("regex: invalid node type");
        }

        return ret;
    }
    void push_node(node_type node) {
        auto& stacktop = _stack.back();
        stacktop.node = push_node_to_node(stacktop.node, node);
    }
    void push_char(char_type c) {
        auto node = std::make_shared<ExprNodeCharRange<char_type>>(c, c);
        push_node(node);
    }
    void to_union_node() {
        auto& stacktop = _stack.back();
        assert (stacktop.node != nullptr);

        switch (stacktop.node->node_type())
        {
            case ExprNodeType_Empty:
            case ExprNodeType_Group:
            case ExprNodeType_CharRange:
            case ExprNodeType_KleeneStar:
            case ExprNodeType_Concatenation: {
                auto newnode = std::make_shared<ExprNodeUnion<char_type>>();
                newnode->add_child(stacktop.node);
                stacktop.node = newnode;
            } break;
            case ExprNodeType_Union: break;
            default:
                assert("regex: invalid node type");
        }
    }
    node_type node_to_kleen_start_node(node_type node) {
        assert(node != nullptr);
        node_type ret;

        switch (node->node_type())
        {
            case ExprNodeType_Group:
            case ExprNodeType_KleeneStar:
            case ExprNodeType_CharRange: {
                auto newnode = std::make_shared<ExprNodeKleeneStar<char_type>>(node);
                ret = newnode;
            } break;
            case ExprNodeType_Concatenation: {
                auto ptr = std::dynamic_pointer_cast<ExprNodeConcatenation<char_type>>(node);
                assert(ptr->size() >= 1);
                auto& back = ptr->back();
                back = node_to_kleen_start_node(back);
                ret = node;
            } break;
            case ExprNodeType_Union: {
                auto ptr = std::dynamic_pointer_cast<ExprNodeUnion<char_type>>(node);
                assert(ptr->size() >= 1);
                auto& back = ptr->back();
                back = node_to_kleen_start_node(back);
                ret = node;
            } break;
            case ExprNodeType_Empty:
                throw std::runtime_error("regex: invalid kleene star node");
            default:
                assert("regex: invalid node type");
        }

        return ret;
    }
    void to_kleene_star_node() {
        auto& stacktop = _stack.back();
        assert(stacktop.node != nullptr);
        stacktop.node = node_to_kleen_start_node(stacktop.node);
        assert(stacktop.node != nullptr);
    }

    bool m_in_bracket_mode;
    bool m_escaping_in_bracket_mode;
    bool m_bracket_reversed;
    size_t m_bracket_eat_count;
    enum BracketState {
        BracketState_None, BracketState_One,
        BracketState_Dashed, BracketState_Two,
    } m_bracket_state;
    std::vector<std::pair<char_type, char_type>> m_bracket_ranges;
    char_type m_range_low, m_range_up;

    void enter_bracket_mode() {
        m_in_bracket_mode = true;
        m_escaping_in_bracket_mode = false;
        m_bracket_eat_count = 0;
        m_bracket_reversed = false;
        m_bracket_state = BracketState_None;
        m_bracket_ranges.clear();
    }
    void leave_bracket_mode() {
        this->push_bracket_range();
        m_in_bracket_mode = false;

        std::sort(m_bracket_ranges.begin(), m_bracket_ranges.end());
        auto merged_ranges = merge_sorted_range(m_bracket_ranges);
        if (merged_ranges.empty())
            throw std::runtime_error("Regex: empty bracket");

        if (merged_ranges.size() == 1) {
            auto node = std::make_shared<ExprNodeCharRange<char_type>>(merged_ranges[0].first, merged_ranges[0].second);
            this->push_node(std::make_shared<ExprNodeGroup<char_type>>(node));
        } else {
            auto node = std::make_shared<ExprNodeUnion<char_type>>();
            for (auto& range : merged_ranges) {
                auto node_range = std::make_shared<ExprNodeCharRange<char_type>>(range.first, range.second);
                node->add_child(node_range);
            }
            this->push_node(std::make_shared<ExprNodeGroup<char_type>>(node));
        }
    }
    void push_bracket_range() {
        switch (this->m_bracket_state) {
            case BracketState_None:
                break;
            case BracketState_One: {
               if (!this->m_bracket_reversed) {
                    this->m_bracket_ranges.push_back(std::make_pair(this->m_range_low, this->m_range_low));
               } else {
                   if (this->m_range_low > traits::MIN)
                        this->m_bracket_ranges.push_back(std::make_pair(traits::MIN, this->m_range_low-1));
                   if (this->m_range_low < traits::MAX)
                        this->m_bracket_ranges.push_back(std::make_pair(this->m_range_low+1, traits::MAX));
               }
            } break;
            case BracketState_Dashed: {
               if (!this->m_bracket_reversed) {
                    this->m_bracket_ranges.push_back(std::make_pair(this->m_range_low, traits::MAX));
               } else {
                   if (this->m_range_low > traits::MIN)
                        this->m_bracket_ranges.push_back(std::make_pair(traits::MIN, this->m_range_low-1));
               }
            } break;
            case BracketState_Two: {
               if (!this->m_bracket_reversed) {
                    this->m_bracket_ranges.push_back(std::make_pair(this->m_range_low, this->m_range_up));
               } else {
                   if (this->m_range_low > traits::MIN)
                        this->m_bracket_ranges.push_back(std::make_pair(traits::MIN, this->m_range_low-1));
                   if (this->m_range_up < traits::MAX)
                        this->m_bracket_ranges.push_back(std::make_pair(this->m_range_up+1, traits::MAX));
               }
               this->m_bracket_state = BracketState_None;
            } break;
            default:
               throw std::runtime_error("invalid bracket state");
        }
    }
    bool handle_bracket_mode(char_type c) {
        if (!this->m_in_bracket_mode)
            return false;

        if (this->m_bracket_eat_count++ == 0 && c == traits::CARET) {
            this->m_bracket_reversed = true;
            return true;
        }

        const bool bracket_end = !this->m_escaping_in_bracket_mode && c == traits::RBRACKET;
        if (bracket_end) {
            this->leave_bracket_mode();
            return true;
        }

        if (this->m_escaping_in_bracket_mode) {
            if (c != traits::RBRACKET && c != traits::BACKSLASH)
                throw std::runtime_error("unexpected escape seqeuence");
            this->m_escaping_in_bracket_mode = false;
        } else if (c == traits::BACKSLASH) {
            this->m_escaping_in_bracket_mode = true;
            return true;
        }

        switch (this->m_bracket_state) {
            case BracketState_None:
            {
                this->m_range_low = c;
                this->m_bracket_state = BracketState_One;
            } break;
            case BracketState_One:
            {
                if (c == traits::DASH) {
                    this->m_bracket_state = BracketState_Dashed;
                } else {
                    this->push_bracket_range();
                    this->m_range_low = c;
                }
            } break;
            case BracketState_Dashed:
            {
                this->m_range_up = c;
                this->m_bracket_state = BracketState_Two;
                this->push_bracket_range();
                this->m_bracket_state = BracketState_None;
            } break;
            default:
                throw std::runtime_error("unexpected bracket state");
        }

        return true;
    }

public:
    RegexNodeTreeGenerator():
       _endding(false), _escaping(false),
        m_in_bracket_mode(false), m_escaping_in_bracket_mode(false),
        _stack()
    {
        this->_stack.push_back(StackValueState());
    }

    void feed(char_type c) {
        assert(!this->_endding);

        if (this->handle_bracket_mode(c))
            return;

        if (this->_escaping) {
            if (c != traits::LPAREN && c != traits::RPAREN &&
                c != traits::LBRACKET && c != traits::CARET &&
                c != traits::DASH && c != traits::RBRACKET &&
                c != traits::OR && c != traits::STAR &&
                c != traits::BACKSLASH)
            {
                throw std::runtime_error("unexpected escape seqeuence");
            }

            this->_escaping = false;
            this->push_char(c);
            return;
        }

        switch (c) {
            case traits::LPAREN:
            {
                this->_stack.push_back(StackValueState());
            } break;
            case traits::RPAREN:
            {
                if (this->_stack.size() < 2)
                    throw std::runtime_error("unexpected ')'");

                auto top = this->_stack.back();
                this->_stack.pop_back();
                auto group = std::make_shared<ExprNodeGroup<char_type>>(top.node);
                this->push_node(group);
            } break;
            case traits::LBRACKET:
            {
                this->enter_bracket_mode();
            } break;
            case traits::RBRACKET:
            {
                throw std::runtime_error("unexpected ]");
            } break;
            case traits::OR:
            {
                this->to_union_node();
                auto& top = this->_stack.back();
                auto node = std::dynamic_pointer_cast<ExprNodeUnion<char_type>>(top.node);
                assert(node != nullptr);
                node->add_child(std::make_shared<ExprNodeEmpty<char_type>>());
            } break;
            case traits::STAR:
            {
                this->to_kleene_star_node();
            } break;
            case traits::BACKSLASH:
            {
                this->_escaping = true;
            } break;
            default:
                this->push_char(c);
        }
    }

    node_type end() {
        if (this->_endding)
            throw std::runtime_error("already endding");

        this->_endding = true;

        if (this->_stack.size() != 1)
            throw std::runtime_error("unexpected end");

        auto top = this->_stack.back();
        this->_stack.pop_back();
        return top.node;
    }

    static node_type parse(const std::vector<char_type>& str) {
        RegexNodeTreeGenerator<char_type> gen;
        for (auto c: str) {
            gen.feed(c);
        }
        return gen.end();
    }
};


// -----------------------------------------------
// template<typename CharT> NFAMatcher implementation
// -----------------------------------------------
template<typename CharT>
NFAMatcher<CharT>::NFAMatcher(const std::vector<char_type>& pattern)
{
    auto nfa = NodeNFA<CharT>::from_regex(pattern);
    auto rnfa = nfa.toRegexNFA();
    this->m_nfa = std::make_shared<RegexNFA<char_type>>(std::move(rnfa));
    this->reset();
}

// -----------------------------------------------
// template<typename CharT> DFAMatcher implementation
// -----------------------------------------------
template<typename CharT>
DFAMatcher<CharT>::DFAMatcher(const std::vector<char_type>& pattern)
{
    auto nfa = NodeNFA<CharT>::from_regex(pattern);
    auto rnfa = nfa.toRegexNFA();
    auto dfa = rnfa.compile();
    this->m_dfa = std::make_shared<RegexDFA<char_type>>(std::move(dfa));
    this->reset();
}

// -----------------------------------------------
// template<typename CharT> NodeNFA implementation
// -----------------------------------------------
/** static */
template<typename CharT>
NodeNFA<CharT> NodeNFA<CharT>::from_basic_regex(const std::vector<char_type>& regex) {
    auto node = RegexNodeTreeGenerator<CharT>::parse(regex);
    StateAllocator allocator;
    auto starts = allocator.newstate();
    auto finals = allocator.newstate();
    auto nfa = node->to_nfa(allocator, starts, finals);
    return nfa;
}

template<typename CharT>
class SimpleRegExp: public AutomataMatcher<CharT>
{
private:
    using traits = character_traits<CharT>;
    using char_type = CharT;
    std::shared_ptr<AutomataMatcher<char_type>> m_matcher;

public:
    SimpleRegExp() = delete;
    SimpleRegExp(const std::vector<char_type>& regex) {
        auto nfa = NodeNFA<char_type>::from_regex(regex);
        auto rnfa = nfa.toRegexNFA();
        auto rnfa_ptr = std::make_shared<RegexNFA<char_type>>(std::move(rnfa));
        this->m_matcher = std::make_shared<NFAMatcher<char_type>>(rnfa_ptr);
    }

    virtual void feed(char_type c) override { std::cout << c << std::endl; this->m_matcher->feed(c); }
    virtual bool match() const override { return this->m_matcher->match(); }
    virtual bool dead() const override { return this->m_matcher->dead(); }
    virtual void reset() override { this->m_matcher->reset(); }
    void compile()
    {
        auto nfa_matcher = std::dynamic_pointer_cast<NFAMatcher<char_type>>(m_matcher);
        if (nfa_matcher == nullptr)
            throw std::runtime_error("unexpected matcher type");

        auto nfa = nfa_matcher->get_nfa();
        auto dfa = std::make_shared<RegexDFA<char_type>>(nfa->compile());
        this->m_matcher = std::make_shared<DFAMatcher<char_type>>(dfa);
    }

    std::string to_string() const
    {
        auto nfa_matcher = std::dynamic_pointer_cast<NFAMatcher<char_type>>(m_matcher);
        auto dfa_matcher = std::dynamic_pointer_cast<DFAMatcher<char_type>>(m_matcher);

        if (nfa_matcher) {
            return nfa_matcher->to_string();
        } else if (dfa_matcher) {
            return dfa_matcher->to_string();
        } else {
            return "bad matcher";
        }
    }
};

#endif // _DC_PARSER_REGEX_HPP_
