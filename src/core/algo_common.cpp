#include "core/algo_common.hpp"

#include <algorithm>
#include <cmath>
#include <format>

namespace gav {

std::string fmtNum(double v) {
    if (std::isinf(v)) return v > 0 ? "∞" : "-∞";
    if (v == std::round(v) && std::abs(v) < 1e9) return std::format("{}", static_cast<long long>(v));
    return std::format("{:.1f}", v);
}

std::string pathString(const Graph& g, const std::vector<int>& path) {
    // 格子地圖的路徑動輒幾十個點，只列頭尾
    constexpr std::size_t Head = 4, Tail = 3;
    std::string s;
    for (std::size_t i = 0; i < path.size(); ++i) {
        if (path.size() > Head + Tail + 2 && i == Head) {
            s += " → …";
            i = path.size() - Tail - 1;
            continue;
        }
        if (i > 0) s += " → ";
        s += g.name(path[i]);
    }
    if (path.size() > Head + Tail + 2) s += std::format("（共 {} 個點）", path.size());
    return s;
}

bool markPath(Recorder& rec, const Graph& g, const std::vector<int>& parentEdge, int src, int dst,
              Result& res) {
    if (src < 0 || dst < 0) return false;
    std::vector<int> nodes{dst};
    std::vector<int> edges;
    int v = dst;
    // 最多走 V 步，避免 parent 有環（例如負環）時卡死
    while (v != src && static_cast<int>(nodes.size()) <= g.size()) {
        const int e = parentEdge[v];
        if (e < 0) return false;
        edges.push_back(e);
        v = g.other(e, v);
        nodes.push_back(v);
    }
    if (v != src) return false;

    std::reverse(nodes.begin(), nodes.end());
    double cost = 0;
    for (int e : edges) {
        cost += g.edges[e].weight;
        rec.edge(e, EdgeState::Path);
    }
    for (int u : nodes) rec.node(u, NodeState::Path);
    res.path = nodes;
    res.pathCost = cost;
    return true;
}

bool allUnitWeights(const Graph& g) {
    return std::all_of(g.edges.begin(), g.edges.end(), [](const Edge& e) { return e.weight == 1; });
}

}  // namespace gav
