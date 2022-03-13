#ifndef _C_PARSER_C_ERROR_H_
#define _C_PARSER_C_ERROR_H_

#include <stdexcept>


namespace cparser {


class CError : public std::runtime_error {
public:
    CError(const std::string& what);
    CError(const char* what);
};

#define CErrorList \
    EEntry(CErrorLexer) \
    EEntry(CErrorParser) \
    EEntry(CErrorStaticAssert) \
    EEntry(CErrorparserMixedType)

#define EEntry(cname) \
    class cname: public CError \
    { \
    public: \
        cname(); \
        cname(const std::string& what); \
    };
CErrorList
#undef EEntry

}

#endif // _C_PARSER_C_ERROR_H_
