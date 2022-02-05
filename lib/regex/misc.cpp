#include "regex/misc.hpp"
using namespace std;


std::vector<std::set<size_t>>
transitive_closure(std::vector<std::set<size_t>> const& graph)
{
    std::vector<std::set<size_t>> result(graph.size());
    for (size_t i = 0; i < graph.size(); ++i) {
        result[i].insert(i);

        for (auto const& j: graph[i]) {
            result[i].insert(j);
            for (auto const& k: graph[j]) {
                result[i].insert(k);
            }
        }
    }
    return result;
}
