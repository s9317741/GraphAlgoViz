// 拓撲排序（Kahn 演算法）：一直拿「沒有先修」的點出來

#include <deque>
#include <format>

#include "core/algo_common.hpp"
#include "core/algorithms.hpp"

namespace gav {

Trace topologicalSort(const Graph& g) {
    Recorder rec(g, "拓撲排序（Kahn）",
                 {
                     "算出每個點的入度（有幾條邊指向它）",
                     "入度為 0 的點（沒有先修）放進佇列",
                     "while 佇列不是空的:",
                     "    u = 從佇列取出，接在結果後面",
                     "    for 每條邊 u→v: 入度[v] -= 1",
                     "        if 入度[v] == 0: v 放進佇列",
                     "結果不到 V 個點 → 圖中有環，排不出來",
                 },
                 "排好的順序 ／ 佇列");
    const int n = g.size();
    std::vector<int> indeg(n, 0);
    for (const Edge& e : g.edges) ++indeg[e.to];
    std::deque<int> queue;
    Result res;

    auto show = [&] {
        std::vector<std::string> items{"結果：" + (res.order.empty() ? std::string("（還沒有）") : pathString(g, res.order))};
        for (int v : queue) items.push_back("佇列：" + g.name(v));
        rec.container(std::move(items));
    };
    auto showIndeg = [&](int v) { rec.label(v, std::format("入度 {}", indeg[v])); };

    for (int u = 0; u < n; ++u) showIndeg(u);
    show();
    rec.step(0, "先算每個點的入度＝有幾條邊指向它（例如有幾門先修課）。入度 0 代表現在就能排。");

    std::string zeros;
    for (int u = 0; u < n; ++u) {
        if (indeg[u] != 0) continue;
        queue.push_back(u);
        rec.node(u, NodeState::Frontier);
        zeros += (zeros.empty() ? "" : "、") + g.name(u);
    }
    show();
    rec.step(1, zeros.empty() ? "沒有任何入度為 0 的點：一開始就卡住了，圖中一定有環。"
                              : std::format("入度為 0 的 {} 沒有任何先修，放進佇列。", zeros));

    while (!queue.empty()) {
        const int u = queue.front();
        queue.pop_front();
        res.order.push_back(u);
        rec.node(u, NodeState::Current);
        show();
        rec.step(3, std::format("取出 {}，排在第 {} 個。", g.name(u), res.order.size()));
        for (int e : g.adj[u]) {
            const int v = g.edges[e].to;
            --indeg[v];
            rec.edge(e, EdgeState::Tree);
            showIndeg(v);
            if (indeg[v] == 0) {
                queue.push_back(v);
                rec.node(v, NodeState::Frontier);
                show();
                rec.step(5, std::format("{} 排好了，{} 少一個先修 → 入度變 0，放進佇列。", g.name(u), g.name(v)));
            } else {
                rec.flashNode(v, NodeState::Current);
                rec.step(4, std::format("{}→{}：{} 的入度減 1，剩 {}（還有別的先修沒排）。", g.name(u), g.name(v),
                                        g.name(v), indeg[v]));
            }
        }
        rec.node(u, NodeState::Visited);
    }

    if (static_cast<int>(res.order.size()) < n) {
        res.hasCycle = true;
        rec.step(6, std::format("只排出 {} 個點，剩下的點入度永遠到不了 0 → 它們之間有環，排不出來。",
                                res.order.size()));
    } else {
        for (int u : res.order) rec.node(u, NodeState::Path);
        rec.step(6, std::format("完成！一種合法的順序：{}。每條邊都是從前面指向後面。", pathString(g, res.order)));
    }
    return rec.finish(std::move(res));
}

}  // namespace gav
