#ifndef _DC_PARSER_REGEX_CHAR_HPP_
#define _DC_PARSER_REGEX_CHAR_HPP_

#include <limits>
#include <string>
#include <vector>


template<typename CharT>
struct character_traits {
    using char_type = CharT;

    constexpr static char_type LPAREN      = '(';
    constexpr static char_type EXCLAMATION = '!';
    constexpr static char_type RPAREN      = ')';
    constexpr static char_type LBRACKET    = '[';
    constexpr static char_type CARET       = '^';
    constexpr static char_type DASH        = '-';
    constexpr static char_type RBRACKET    = ']';
    constexpr static char_type OR          = '|';
    constexpr static char_type STAR        = '*';
    constexpr static char_type BACKSLASH   = '\\';

    constexpr static char_type LBRACE      = '{';
    constexpr static char_type COMMA       = ',';
    constexpr static char_type RBRACE      = '}';
    constexpr static char_type QUESTION    = '?';
    constexpr static char_type PLUS        = '+';
    constexpr static char_type DOT         = '.';

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

template<typename CharT>
class RegexPatternChar {
private:
    using traits = character_traits<CharT>;
    bool m_escaping;
    CharT m_char;

    RegexPatternChar(bool escaping, CharT c) : m_escaping(escaping), m_char(c) {}

public:
    RegexPatternChar() : m_escaping(false), m_char(traits::EMPTY_CHAR) {}

    static RegexPatternChar<CharT> unescape(CharT c) {
        return RegexPatternChar<CharT>(false, c);
    }
    static RegexPatternChar<CharT> escape(CharT c) {
        return RegexPatternChar<CharT>(true, c);
    }

    bool is_escaped() const {
        return m_escaping;
    }

    CharT get() const {
        return m_char;
    }

    std::string to_string() const {
        if (m_escaping) {
            return char_to_string(traits::BACKSLASH) + char_to_string(m_char);
        } else {
            return char_to_string(m_char);
        }
    }
};

template<typename CharT>
std::string pattern_to_string(std::vector<RegexPatternChar<CharT>> pattern) {
    std::string result;
    for (auto c : pattern) {
        result += c.to_string();
    }
    return result;
}

#endif // _DC_PARSER_REGEX_CHAR_HPP_
