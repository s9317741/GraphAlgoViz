#pragma once

#include <string>
#include <vector>

namespace gav {

struct Vec2 {
    float x = 0;
    float y = 0;
};

struct Node {
    std::string name;
    Vec2 pos;  // 畫圖用的座標；A* 的啟發函數也拿它算直線距離
};

struct Edge {
    int from = 0;
    int to = 0;
    double weight = 0;
};

// 鄰接串列（adjacency list）表示法。
// 無向圖的一條邊在 edges 裡只存一筆，但兩個端點的 adj 都會列出它。
struct Graph {
    bool directed = false;
    std::vector<Node> nodes;
    std::vector<Edge> edges;
    std::vector<std::vector<int>> adj;  // adj[u] = 從 u 出發能走的邊編號

    int addNode(std::string name, Vec2 pos);
    int addEdge(int from, int to, double weight);

    int size() const { return static_cast<int>(nodes.size()); }
    const std::string& name(int u) const { return nodes[u].name; }

    // 從 u 沿著邊 e 走過去會到哪個點
    int other(int e, int u) const;
};

}  // namespace gav
