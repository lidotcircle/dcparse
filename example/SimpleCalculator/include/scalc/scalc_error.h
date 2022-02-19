#ifndef _SIMPLE_CALCULATOR_SCALC_ERROR_H_
#define _SIMPLE_CALCULATOR_SCALC_ERROR_H_

#include <stdexcept>

class SCalcError : public std::runtime_error {
public:
    SCalcError(const std::string& what);
};

#define SCalcErrorList \
    EEntry(SCalcBadBreakStatement) \
    EEntry(SCalcBadContinueStatement) \
    EEntry(SCalcBadReturnStatement) \
    EEntry(SCalcBadFunctionArgumentCount) \
    EEntry(SCalcUndefinedFunction) \
    EEntry(SCalcBadFunctionNameExpression) \
    EEntry(SCalcExpectAnLValue)

#define EEntry(cname) \
    class cname: public SCalcError \
    { \
    public: \
        cname(); \
        cname(const std::string& what); \
    };
SCalcErrorList
#undef EEntry

#endif // _SIMPLE_CALCULATOR_SCALC_ERROR_H_
