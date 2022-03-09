#include "c_reporter.h"
#include <sstream>
#include <iomanip>
#include <map>
using namespace cparser;
using namespace std;
using ErrorLevel = SemanticError::ErrorLevel;


SemanticError::SemanticError(
        const string& what,
        size_t spos, size_t epos,
        shared_ptr<TokenPositionInfo> posinfo): 
    CError(what), _start_pos(spos), _end_pos(epos), _pos_info(posinfo)
{
}

size_t SemanticError::start_pos() const { return this->_start_pos; }
size_t SemanticError::end_pos()   const { return this->_end_pos; }

static map<string,string> ANSI_MAP = {
    {"black",     "\033[30m"},
    {"red",       "\033[31m"},
    {"green",     "\033[32m"},
    {"yellow",    "\033[33m"},
    {"blue",      "\033[34m"},
    {"magenta",   "\033[35m"},
    {"cyan",      "\033[36m"},
    {"white",     "\033[37m"},

    {"underline", "\033[4m"},
    {"bold",      "\033[1m"},
    {"italic",    "\033[3m"},
    {"blink",     "\033[5m"},
    {"reverse",   "\033[7m"},
    {"hidden",    "\033[8m"},

    {"reset",     "\033[0m"},
};

static map<ErrorLevel,string> error2color = {
    { ErrorLevel::INFO,    "green" },
    { ErrorLevel::WARNING, "yellow" },
    { ErrorLevel::ERROR,   "red" },
};

string SemanticError::error_message() const
{
    ostringstream oss;

    oss << ANSI_MAP["bold"] << this->_pos_info->queryLine(this->_start_pos);
    oss << ": " << ANSI_MAP[error2color[this->error_level()]] << this->error_type() << ": ";
    oss << ANSI_MAP["reset"] << ANSI_MAP["bold"] <<  this->what() << ANSI_MAP["reset"] << endl;

    auto linfo = this->_pos_info->query(this->_start_pos);
    auto lines = this->_pos_info->lines(this->_start_pos, this->_end_pos);
    size_t ln = linfo.line;
    for (auto& l: lines) {
        oss << ANSI_MAP["white"] << std::setw(5) << std::setfill(' ') << ln << " | ";
        oss << ANSI_MAP["bold"] << l.line.substr(0, l.beg);
        oss << ANSI_MAP[error2color[this->error_level()]] << l.line.substr(l.beg, l.end) << ANSI_MAP["reset"];
        oss << ANSI_MAP["bold"] << l.line.substr(l.end) << ANSI_MAP["reset"] << endl;
    }

    return oss.str();
}
