#ifndef _DCPARSER_ALGO_HPP_
#define _DCPARSER_ALGO_HPP_

#include <map>
#include <set>

template<typename T>
std::map<T,std::set<T>>
transitive_closure(std::map<T,std::set<T>> graph)
{
    std::map<T,std::set<T>> result;
    for (const auto& kv: graph) {
        const auto& key = kv.first;
        result[key].insert(key);

        for (const auto& j: graph[key]) {
            result[key].insert(j);
            for (const auto& k: graph[j]) {
                result[key].insert(k);
            }
        }
    }
    return result;
}

#endif // _DCPARSER_ALGO_HPP_
