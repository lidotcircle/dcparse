#include "c_reporter.h"
#include <iomanip>
#include <map>
#include <sstream>
#include <string.h>
using namespace cparser;
using namespace std;
using ErrorLevel = SemanticError::ErrorLevel;


SemanticError::SemanticError(const string& what,
                             const TextRangeEntity& range,
                             shared_ptr<TextInfo> textinfo)
    : CError(what), TextRangeEntity(range), _pos_info(textinfo)
{}

static map<string, string> ANSI_MAP = {
    {"black", "\033[30m"},
    {"red", "\033[31m"},
    {"green", "\033[32m"},
    {"yellow", "\033[33m"},
    {"blue", "\033[34m"},
    {"magenta", "\033[35m"},
    {"cyan", "\033[36m"},
    {"white", "\033[37m"},

    {"underline", "\033[4m"},
    {"bold", "\033[1m"},
    {"italic", "\033[3m"},
    {"blink", "\033[5m"},
    {"reverse", "\033[7m"},
    {"hidden", "\033[8m"},

    {"reset", "\033[0m"},
};

static map<ErrorLevel, string> error2color = {
    {ErrorLevel::INFO, "cyan"},
    {ErrorLevel::WARNING, "magenta"},
    {ErrorLevel::ERROR, "red"},
};

string SemanticError::error_message() const
{
    ostringstream oss;

    size_t beg = 0, end = 0;
    if (this->range().has_value()) {
        beg = this->beg().value();
        end = this->end().value();
    }

    oss << ANSI_MAP["bold"] << this->_pos_info->row_col_str(beg);
    oss << ": " << this->error_type() << " " << ANSI_MAP[error2color[this->error_level()]]
        << this->error_level_text() << ": ";
    oss << ANSI_MAP["reset"] << ANSI_MAP["bold"] << CError::what() << ANSI_MAP["reset"] << endl;

    auto linfo = this->_pos_info->query(beg);
    auto lines = this->_pos_info->lines(beg, end);
    size_t ln = linfo.line;
    for (auto& l : lines) {
        if (!l.line.empty() && l.line.back() == '\n') {
            if (l.line.size() == l.end)
                l.end--;
            l.line.pop_back();
        }

        oss << ANSI_MAP["white"] << std::setw(5) << std::setfill(' ') << ln << " | ";
        oss << ANSI_MAP["bold"] << l.line.substr(0, l.beg);
        oss << ANSI_MAP[error2color[this->error_level()]] << l.line.substr(l.beg, l.end - l.beg)
            << ANSI_MAP["reset"];
        oss << ANSI_MAP["bold"] << l.line.substr(l.end) << ANSI_MAP["reset"] << endl;
        ln++;
    }

    return oss.str();
}

const char* SemanticError::what() const noexcept
{
    auto _this = const_cast<SemanticError*>(this);
    _this->_error_buf = this->error_message();
    return this->_error_buf.c_str();
}

#define SENTRY(name, level, type)                                                                  \
    SemanticError##name::SemanticError##name(                                                      \
        const string& what, const TextRangeEntity& range, shared_ptr<TextInfo> textinfo)           \
        : SemanticError(what, range, textinfo)                                                     \
    {}                                                                                             \
    string SemanticError##name::error_type() const                                                 \
    {                                                                                              \
        return type;                                                                               \
    }                                                                                              \
    ErrorLevel SemanticError##name::error_level() const                                            \
    {                                                                                              \
        return ErrorLevel::level;                                                                  \
    }                                                                                              \
    const char* SemanticError##name::error_level_text() const                                      \
    {                                                                                              \
        return #level;                                                                             \
    }
SEMANTIC_ERROR_LIST
#undef SENTRY


SemanticReporter::SemanticReporter() : _error_count(0), _warning_count(0), _info_count(0)
{}

vector<shared_ptr<SemanticError>> SemanticReporter::get_errors() const
{
    vector<shared_ptr<SemanticError>> ret;
    for (auto msg : *this) {
        if (msg->error_level() == ErrorLevel::ERROR)
            ret.push_back(msg);
    }
    return ret;
}

vector<shared_ptr<SemanticError>> SemanticReporter::get_warnings() const
{
    vector<shared_ptr<SemanticError>> ret;
    for (auto msg : *this) {
        if (msg->error_level() == ErrorLevel::WARNING)
            ret.push_back(msg);
    }
    return ret;
}

vector<shared_ptr<SemanticError>> SemanticReporter::get_infos() const
{
    vector<shared_ptr<SemanticError>> ret;
    for (auto msg : *this) {
        if (msg->error_level() == ErrorLevel::INFO)
            ret.push_back(msg);
    }
    return ret;
}
