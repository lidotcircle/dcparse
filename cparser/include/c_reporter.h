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

public:
    SemanticError(const std::string& what,
                  size_t start_pos, size_t end_pos, 
                  std::shared_ptr<TokenPositionInfo> posinfo);

    size_t start_pos() const;
    size_t end_pos() const;

    virtual std::string error_type() const = 0;
    virtual ErrorLevel  error_level() const = 0;
    virtual std::string error_message() const;
};

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
