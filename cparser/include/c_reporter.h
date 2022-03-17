#ifndef _C_PARSER_C_REPORTER_H_
#define _C_PARSER_C_REPORTER_H_

#include <string>
#include <vector>
#include <memory>
#include "c_error.h"
#include "lexer/text_info.h"


namespace cparser {


class SemanticError: public CError, public TextRangeEntity {
public:
    enum ErrorLevel { INFO, WARNING, ERROR };

private:
    std::shared_ptr<TextInfo> _pos_info;
    std::string _error_buf;

public:
    SemanticError(const std::string& what,
                  const TextRangeEntity& range,
                  std::shared_ptr<TextInfo> textinfo);

    virtual std::string error_type() const = 0;
    virtual ErrorLevel  error_level() const = 0;
    virtual const char* error_level_text() const = 0;
    virtual std::string error_message() const;
    virtual const char* what() const noexcept override;
};


#define SEMANTIC_ERROR_LIST \
    SENTRY(None,                       ERROR, "none") \
    SENTRY(Redefinition,               ERROR, "redefinition") \
    SENTRY(NonConstant,                ERROR, "expect a constant") \
    SENTRY(BadType,                    ERROR, "unexpected type") \
    SENTRY(VarNotDefined,              ERROR, "variable not defined") \
    SENTRY(InvalidArrayIndex,          ERROR, "invalid array index") \
    SENTRY(InvalidFunctionCall,        ERROR, "invalid function call") \
    SENTRY(InvalidMemberAccess,        ERROR, "invalid member access") \
    SENTRY(InvalidValueCategory,       ERROR, "invalid value category") \
    SENTRY(InvalidOperand,             ERROR, "invalid operand") \
    SENTRY(InvalidCastFailed,          ERROR, "cast failed") \
    SENTRY(InvalidInitializer,         ERROR, "invalid initializer expression") \
    SENTRY(ModifyConstant,             ERROR, "try to modify a constant") \
    SENTRY(DuplicateMember,            ERROR, "duplicate member") \
    SENTRY(IncompleteType,             ERROR, "incomplete type") \
    SENTRY(IncompatibleTypes,          ERROR, "incompatible types") \
    SENTRY(InvalidFunctionParameter,   ERROR, "invalid function parameter") \
    SENTRY(DuplicateCaseLabel,         ERROR, "duplicate case label") \
    SENTRY(UndefinedLabel,             ERROR, "undefined label") \
    SENTRY(ContinueOutsideLoop,        ERROR, "continue statement outside loop") \
    SENTRY(BreakOutsideLoop,           ERROR, "break statement outside loop or switch") \
    SENTRY(MissingReturnValue,         ERROR, "missing return value") \
    SENTRY(StaticAssertFailed,         ERROR, "static_assert failed") \
    SENTRY(RequireIntegerType,         ERROR, "require integer type") \
    SENTRY(BitFieldIsTooLarge,         ERROR, "bit-field is too large") \
    SENTRY(NegativeBitField,           ERROR, "negative bit-field specifier") \
    SENTRY(InvalidInitializerList,     ERROR, "invalid initializer list") \
    SENTRY(InvalidDesignation,         ERROR, "invalid initializer designation") \
    SENTRY(InvalidArrayDesignator,     ERROR, "invalid array designator") \
    \
    SENTRY(EmptyStructUnionDefinition, WARNING, "empty struct or union definition") \
    SENTRY(InitializerStringTooLong,   WARNING, "initializer-string for char array is too long") \


#define SENTRY(name, _, __) \
    class SemanticError##name: public SemanticError { \
    public: \
        SemanticError##name(const std::string& what, const TextRangeEntity& range, \
                            std::shared_ptr<TextInfo> textinfo); \
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
    using ErrorLevel = SemanticError::ErrorLevel;
    size_t _error_count, _warning_count, _info_count;

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
    using container_t::operator[];

    template<typename T>
    void push_back(T v)
    {
        auto errlevel = v->error_level();
        if (errlevel == ErrorLevel::ERROR)
            this->_error_count++;
        if (errlevel == ErrorLevel::WARNING)
            this->_warning_count++;
        if (errlevel == ErrorLevel::INFO)
            this->_info_count++;
        container_t::push_back(v);
    }

    inline size_t error_count() const { return this->_error_count; }
    inline size_t warning_count() const { return this->_warning_count; }
    inline size_t info_count() const { return this->_info_count; }

    container_t get_errors() const;
    container_t get_warnings() const;
    container_t get_infos() const;
};
}

#endif // _C_PARSER_C_REPORTER_H_
