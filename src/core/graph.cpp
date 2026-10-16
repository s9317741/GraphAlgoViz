#include "core/graph.hpp"

#include <utility>

namespace gav {

int Graph::addNode(std::string name, Vec2 pos) {
    nodes.push_back({std::move(name), pos});
    adj.emplace_back();
    return size() - 1;
}

int Graph::addEdge(int from, int to, double weight) {
    const int id = static_cast<int>(edges.size());
    edges.push_back({from, to, weight});
    adj[from].push_back(id);
    if (!directed && from != to) adj[to].push_back(id);
    return id;
}

int Graph::other(int e, int u) const {
    const Edge& ed = edges[e];
    return ed.from == u ? ed.to : ed.from;
}

}  // namespace gav
