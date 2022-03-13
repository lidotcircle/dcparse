#ifndef _LEXER_POSITION_INFO_H_
#define _LEXER_POSITION_INFO_H_

#include <string>
#include <vector>


class TokenPositionInfo {
public:
    struct PInfo {
        size_t line;
        size_t column;
    };
    struct LineRange {
        std::string line;
        size_t beg, end;
    };

    virtual const std::string& filename() const = 0;
    virtual size_t len() const = 0;
    virtual PInfo query(size_t pos) const = 0;
    virtual std::pair<size_t,size_t> line_range(size_t line) const = 0;
    virtual std::string query_string(size_t from, size_t to) const = 0;
    inline std::string query_line(size_t line) const {
        auto range = line_range(line);
        return query_string(range.first, range.second);
    }

    std::string queryLine(size_t pos) const;
    std::string queryLine(size_t pos, size_t end_pos) const;

    std::vector<LineRange> lines(size_t beg_pos, size_t end_pos) const;
};

#endif // _LEXER_POSITION_INFO_H_
