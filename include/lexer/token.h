#ifndef _LEXER_TOKEN_H_
#define _LEXER_TOKEN_H_

#include <string>
#include <type_traits>

class DChar {
public:
    virtual const char* charname() const;
    virtual size_t charid() const;
};

template<typename T, typename = std::enable_if_t<std::is_base_of<DChar,T>::value>>
size_t CharID() { return typeid(T).hash_code(); }

class LexerToken : public DChar {
private:
    size_t      m_line;
    size_t      m_column;
    size_t      m_pos;
    size_t      m_len;
    std::string m_filename;

public:
    LexerToken(size_t ln, size_t cn, size_t pos, size_t len, const std::string& fn);

    virtual size_t line_number() const;
    virtual size_t column_number() const;
    virtual size_t position() const;
    virtual size_t length() const;
    virtual const std::string& filename() const;

    virtual ~LexerToken() = default;
};

#endif // _LEXER_TOKEN_H_
