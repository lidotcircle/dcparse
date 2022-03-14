#include "lexer/position_info.h"
#include "assert.h"
using namespace std;
using PInfo = TokenPositionInfo::PInfo;
using LineRange = TokenPositionInfo::LineRange;


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

vector<LineRange> TokenPositionInfo::lines(size_t beg_pos, size_t end_pos) const
{
    assert(beg_pos <= end_pos);
    if (beg_pos == end_pos)
        return {};
    end_pos--;

    auto binfo = this->query(beg_pos);
    auto einfo = this->query(end_pos);
    vector<LineRange> ret;

    for (auto bl=binfo.line;bl<=einfo.line;bl++) {
        auto l = this->query_line(bl);
        size_t b = 0, e = l.size();
        if (bl == binfo.line)
            b = binfo.column - 1;
        if (bl == einfo.line)
            e = einfo.column;

        ret.push_back(LineRange{ .line = l, .beg = b, .end = e });
    }

    return ret;
}
