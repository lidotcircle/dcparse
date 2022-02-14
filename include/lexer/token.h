#ifndef _LEXER_TOKEN_H_
#define _LEXER_TOKEN_H_

#include <string>
#include <type_traits>

class DChar {
public:
    virtual const char* charname() const;
    virtual size_t charid() const;
    virtual ~DChar() = default;
};

template<typename T, typename = std::enable_if_t<std::is_base_of<DChar,T>::value>>
size_t CharID() { return typeid(T).hash_code(); }

class LexerToken : public DChar {
public:
    struct TokenInfo {
        size_t      line_num;
        size_t      column_num;
        size_t      pos;
        size_t      len;
        std::string filename;

        TokenInfo() = default;
        inline TokenInfo(size_t line_num, size_t column_num, size_t pos, size_t len, std::string filename)
            : line_num(line_num), column_num(column_num), pos(pos), len(len), filename(std::move(filename)) {}
    };

private:
    TokenInfo m_info;

public:
    LexerToken(TokenInfo info);

    virtual size_t line_number() const;
    virtual size_t column_number() const;
    virtual size_t position() const;
    virtual size_t length() const;
    virtual const std::string& filename() const;

    virtual ~LexerToken() = default;
};

#endif // _LEXER_TOKEN_H_
