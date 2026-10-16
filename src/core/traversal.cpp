// 圖的走訪：BFS（廣度優先）與 DFS（深度優先）

#include <deque>
#include <format>
#include <functional>

#include "core/algo_common.hpp"
#include "core/algorithms.hpp"

namespace gav {

Trace bfs(const Graph& g, int src, int dst) {
    Recorder rec(g, "BFS 廣度優先搜尋",
                 {
                     "把起點放進佇列，層數 = 0",
                     "while 佇列不是空的:",
                     "    u = 從佇列前端取出",
                     "    if u 是終點: 找到了，結束",
                     "    for u 的每個鄰居 v:",
                     "        if v 已經發現過: 跳過",
                     "        層數[v] = 層數[u] + 1，記下 v 從 u 來",
                     "        v 放進佇列尾端",
                     "沿著「從哪來」往回走，得到路徑",
                 },
                 "佇列（先進先出）");
    const int n = g.size();
    std::vector<double> level(n, INF);
    std::vector<int> parentEdge(n, -1);
    std::deque<int> queue;
    Result res;

    auto showQueue = [&] {
        std::vector<std::string> items;
        for (int v : queue) items.push_back(std::format("{}（第 {} 層）", g.name(v), fmtNum(level[v])));
        rec.container(std::move(items));
    };

    level[src] = 0;
    queue.push_back(src);
    rec.node(src, NodeState::Frontier);
    rec.label(src, "0");
    showQueue();
    rec.step(0, std::format("把起點 {} 放進佇列。BFS 一層一層往外擴張，點旁的數字是第幾層（離起點幾條邊）。",
                            g.name(src)));

    bool found = false;
    while (!queue.empty()) {
        const int u = queue.front();
        queue.pop_front();
        res.order.push_back(u);
        rec.node(u, NodeState::Current);
        showQueue();
        rec.step(2, std::format("從佇列前端取出 {}（第 {} 層）。先進佇列的先處理，所以近的一層一定比遠的一層先處理完。",
                                g.name(u), fmtNum(level[u])));
        if (u == dst) {
            found = true;
            rec.step(3, "取出的就是終點！BFS 第一次碰到終點時，經過的邊數一定最少。");
            break;
        }
        for (int e : g.adj[u]) {
            const int v = g.other(e, u);
            if (level[v] != INF) {
                rec.flashEdge(e, EdgeState::Examining);
                rec.step(5, std::format("{} 已經發現過了，跳過。", g.name(v)));
                continue;
            }
            level[v] = level[u] + 1;
            parentEdge[v] = e;
            queue.push_back(v);
            rec.node(v, NodeState::Frontier);
            rec.label(v, fmtNum(level[v]));
            rec.edge(e, EdgeState::Tree);
            showQueue();
            rec.step(7, std::format("發現 {}：第 {} 層，從 {} 走過來，放進佇列尾端。", g.name(v),
                                    fmtNum(level[v]), g.name(u)));
        }
        rec.node(u, NodeState::Visited);
    }

    res.dist = level;
    if (dst < 0) {
        rec.step(1, std::format("佇列空了，所有走得到的點都拜訪過了（共 {} 個）。", res.order.size()));
    } else if (found && markPath(rec, g, parentEdge, src, dst, res)) {
        std::string text = std::format("路徑：{}，共 {} 條邊。", pathString(g, res.path), res.path.size() - 1);
        if (!allUnitWeights(g))
            text += std::format("注意 BFS 不看權重：這條路權重加起來是 {}，不一定最便宜（換 Dijkstra 試試）。",
                                fmtNum(res.pathCost));
        rec.step(8, text);
    } else {
        rec.step(1, "佇列空了還沒碰到終點 → 終點走不到。");
    }
    return rec.finish(std::move(res));
}

Trace dfs(const Graph& g, int src, int dst) {
    Recorder rec(g, "DFS 深度優先搜尋",
                 {
                     "DFS(u):",
                     "    標記 u 已拜訪",
                     "    if u 是終點: 找到了，一路返回",
                     "    for u 的每個鄰居 v:",
                     "        if v 已拜訪: 跳過",
                     "        記下 v 從 u 來，DFS(v)  ← 先一路走到底",
                     "    u 的鄰居都試過了，回溯到上一層",
                     "沿著「從哪來」往回走，得到路徑",
                 },
                 "遞迴堆疊（最上面是現在這層）");
    const int n = g.size();
    std::vector<bool> visited(n, false);
    std::vector<int> parentEdge(n, -1);
    std::vector<int> stack;
    Result res;

    auto showStack = [&] {
        std::vector<std::string> items;
        for (auto it = stack.rbegin(); it != stack.rend(); ++it) items.push_back(std::format("DFS({})", g.name(*it)));
        rec.container(std::move(items));
    };

    // 用遞迴寫最貼近課本；遞迴堆疊就是右邊顯示的那一疊
    std::function<bool(int)> visit = [&](int u) -> bool {
        visited[u] = true;
        res.order.push_back(u);
        stack.push_back(u);
        rec.node(u, NodeState::Current);
        rec.label(u, std::format("#{}", res.order.size()));
        showStack();
        if (u == src)
            rec.step(1, std::format("從起點 {} 開始，標記為已拜訪。點旁的 #k 表示第 k 個被拜訪。", g.name(u)));
        else
            rec.step(1, std::format("走到 {}，標記為已拜訪（第 {} 個）。", g.name(u), res.order.size()));

        if (u == dst) {
            rec.step(2, "到達終點！DFS 找到的是「第一條碰到的路」，不保證最短。");
            return true;
        }
        for (int e : g.adj[u]) {
            const int v = g.other(e, u);
            if (visited[v]) {
                rec.flashEdge(e, EdgeState::Examining);
                rec.step(4, std::format("{} 已經拜訪過了，跳過。", g.name(v)));
                continue;
            }
            parentEdge[v] = e;
            rec.edge(e, EdgeState::Tree);
            rec.node(u, NodeState::Frontier);  // u 還在堆疊裡，等 v 那邊走完再回來
            if (visit(v)) return true;
            rec.node(u, NodeState::Current);
            showStack();
            rec.step(3, std::format("回到 {}，繼續試它的下一個鄰居。", g.name(u)));
        }
        stack.pop_back();
        rec.node(u, NodeState::Visited);
        showStack();
        rec.step(6, std::format("{} 的鄰居都試過了，回溯到上一層。", g.name(u)));
        return false;
    };

    const bool found = visit(src);
    if (dst < 0) {
        rec.step(0, std::format("全部走完了，共拜訪 {} 個點。", res.order.size()));
    } else if (found && markPath(rec, g, parentEdge, src, dst, res)) {
        rec.step(7, std::format("路徑：{}（{} 條邊，權重和 {}）。DFS 只保證找得到，不保證最短。",
                                pathString(g, res.path), res.path.size() - 1, fmtNum(res.pathCost)));
    } else {
        rec.step(0, "所有走得到的點都試過了，沒碰到終點 → 終點走不到。");
    }
    return rec.finish(std::move(res));
}

}  // namespace gav
