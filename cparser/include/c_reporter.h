#ifndef _C_PARSER_C_REPORTER_H_
#define _C_PARSER_C_REPORTER_H_

#include <string>
#include <vector>
#include <memory>
#include "c_error.h"
#include "lexer/position_info.h"


namespace cparser {


class SemanticError: public CError {
public:
    enum ErrorLevel { INFO, WARNING, ERROR };

private:
    size_t _start_pos, _end_pos;
    std::shared_ptr<TokenPositionInfo> _pos_info;
    std::string _error_buf;

public:
    SemanticError(const std::string& what,
                  size_t start_pos, size_t end_pos, 
                  std::shared_ptr<TokenPositionInfo> posinfo);

    size_t start_pos() const;
    size_t end_pos() const;

    virtual std::string error_type() const = 0;
    virtual ErrorLevel  error_level() const = 0;
    virtual const char* error_level_text() const = 0;
    virtual std::string error_message() const;
    virtual const char* what() const noexcept override;
};


#define SEMANTIC_ERROR_LIST \
    SENTRY(None,                 ERROR, "none") \
    SENTRY(Redefinition,         ERROR, "redefinition") \
    SENTRY(NonConstant,          ERROR, "expect a constant") \
    SENTRY(BadType,              ERROR, "unexpected type") \
    SENTRY(VarNotDefined,        ERROR, "variable not defined") \
    SENTRY(InvalidArrayIndex,    ERROR, "invalid array index") \
    SENTRY(InvalidFunctionCall,  ERROR, "invalid function call") \
    SENTRY(InvalidMemberAccess,  ERROR, "invalid member access") \
    SENTRY(InvalidValueCategory, ERROR, "invalid value category") \
    SENTRY(InvalidOperand,       ERROR, "invalid operand") \
    SENTRY(InvalidCastFailed,    ERROR, "cast failed") \
    SENTRY(InvalidInitializer,   ERROR, "invalid initializer expression") \
    SENTRY(ModifyConstant,       ERROR, "try to modify a constant") \
    SENTRY(DuplicateMember,      ERROR, "duplicate member") \
    SENTRY(IncompleteType,       ERROR, "incomplete type") \
    SENTRY(IncompatibleTypes,    ERROR, "incompatible types") \
    SENTRY(DuplicateCaseLabel,   ERROR, "duplicate case label") \
    SENTRY(UndefinedLabel,       ERROR, "undefined label") \
    SENTRY(ContinueOutsideLoop,  ERROR, "continue statement outside loop") \
    SENTRY(BreakOutsideLoop,     ERROR, "break statement outside loop or switch") \
    SENTRY(MissingReturnValue,   ERROR, "missing return value") \
    SENTRY(StaticAssertFailed,   ERROR, "static_assert failed") \


#define SENTRY(name, _, __) \
    class SemanticError##name: public SemanticError { \
    public: \
        SemanticError##name(const std::string& what, size_t start_pos, size_t end_pos, \
                            std::shared_ptr<TokenPositionInfo> posinfo); \
        virtual std::string error_type() const override; \
        virtual ErrorLevel  error_level() const override; \
        virtual const char* error_level_text() const override; \
    };
SEMANTIC_ERROR_LIST
#undef SENTRY


class SemanticReporter: private std::vector<std::shared_ptr<SemanticError>>
{
private:
    using container_t = std::vector<std::shared_ptr<SemanticError>>;

public:
    SemanticReporter();

    using reference = container_t::reference;
    using const_reference = container_t::const_reference;
    using iterator = container_t::iterator;
    using const_iterator = container_t::const_iterator;

    using container_t::begin;
    using container_t::end;
    using container_t::size;
    using container_t::empty;
    using container_t::push_back;
    using container_t::operator[];
};
}

#endif // _C_PARSER_C_REPORTER_H_
