#ifndef _DC_PARSER_REGEX_MISC_HPP_
#define _DC_PARSER_REGEX_MISC_HPP_

#include <algorithm>
#include <vector>


template<typename T>
std::vector<std::pair<T,T>> merge_sorted_range(const std::vector<std::pair<T,T>>& ranges) {
    std::vector<std::pair<T,T>> result;
    for (auto const& range : ranges) {
        if (result.empty() || result.back().second < range.first) {
            result.push_back(range);
        } else {
            result.back().second = std::max(result.back().second, range.second);
        }
    }
    return result;
}


#endif // _DC_PARSER_REGEX_MISC_HPP_
