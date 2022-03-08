#include "lexer/position_info.h"
using namespace std;
using PInfo = TokenPositionInfo::PInfo;


string TokenPositionInfo::queryLine(size_t pos) const
{
    auto px = this->query(pos);
    auto str = "line " + to_string(px.line) + ":" + to_string(px.column);
    if (this->filename().size() > 0)
        str += this->filename() + "[" + str + "]";
    return str;
}

string TokenPositionInfo::queryLine(size_t p1, size_t p2) const
{
    auto px = this->query(p1);
    auto py = this->query(p2);
    auto s = "line " + to_string(px.line) + ":" + to_string(px.column) + "-" +
                       to_string(py.line) + ":" + to_string(py.column);
    if (this->filename().size() > 0)
        s += this->filename() + "[" + s + "]";
    return s;
}
