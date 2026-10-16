// 最短路徑：Dijkstra、A*、Bellman-Ford、Floyd-Warshall

#include <algorithm>
#include <cassert>
#include <format>

#include "core/algo_common.hpp"
#include "core/algorithms.hpp"

namespace gav {

// ---------------------------------------------------------------------------
// Dijkstra：每次挑「目前已知距離最小」的點，它的距離就確定了。
// 前提是邊權重都不是負的。
// ---------------------------------------------------------------------------
Trace dijkstra(const Graph& g, int src, int dst) {
    Recorder rec(g, "Dijkstra 最短路徑",
                 {
                     "dist[所有點] = ∞，dist[起點] = 0，起點放進優先佇列",
                     "while 優先佇列不是空的:",
                     "    u = 取出 dist 最小的點",
                     "    if u 已經確定: 跳過（過期的舊紀錄）",
                     "    u 的最短距離確定了",
                     "    if u 是終點: 結束",
                     "    for 每條從 u 出發的邊 u→v（權重 w）:",
                     "        if dist[u] + w < dist[v]:",
                     "            dist[v] = dist[u] + w，記下 v 從 u 來，v 放進佇列",
                     "沿著「從哪來」往回走，得到路徑",
                 },
                 "優先佇列（距離小的先出來）");
    const int n = g.size();
    std::vector<double> dist(n, INF);
    std::vector<int> parentEdge(n, -1);
    std::vector<bool> done(n, false);
    Result res;

    struct Item {
        double d;
        long long seq;  // 距離相同時先放進去的先出來，讓動畫結果固定
        int v;
    };
    struct ItemLess {
        bool operator()(const Item& a, const Item& b) const { return a.d != b.d ? a.d < b.d : a.seq < b.seq; }
    };
    MinHeap<Item, ItemLess> pq;
    long long seq = 0;

    auto showQueue = [&] {
        std::vector<std::string> items;
        for (const Item& it : pq.sorted()) {
            const bool stale = done[it.v] || it.d > dist[it.v];
            items.push_back(std::format("{}：{}{}", g.name(it.v), fmtNum(it.d), stale ? "（過期）" : ""));
        }
        rec.container(std::move(items));
    };

    for (int u = 0; u < n; ++u) rec.label(u, "∞");
    dist[src] = 0;
    pq.push({0, seq++, src});
    rec.node(src, NodeState::Frontier);
    rec.label(src, "0");
    showQueue();
    rec.step(0, "一開始只知道起點到自己的距離是 0，其他點都是 ∞（還不知道怎麼到）。點旁的數字是目前找到的最短距離。");

    bool found = false;
    while (!pq.empty()) {
        const Item it = pq.pop();
        const int u = it.v;
        if (done[u]) {
            showQueue();
            rec.step(3, std::format("取出 {}：{}，但 {} 的距離早就確定了，這是之前留下的舊紀錄，丟掉。",
                                    g.name(u), fmtNum(it.d), g.name(u)));
            continue;
        }
        done[u] = true;
        rec.node(u, NodeState::Current);
        showQueue();
        rec.step(4, std::format("取出距離最小的 {}（{}）。佇列裡其他點都 >= {}，邊又不是負的，"
                                "不可能再繞出更短的路 → {} 的最短距離確定了。",
                                g.name(u), fmtNum(dist[u]), fmtNum(dist[u]), g.name(u)));
        if (u == dst) {
            found = true;
            rec.step(5, "終點的距離確定了，結束。");
            break;
        }
        for (int e : g.adj[u]) {
            const int v = g.other(e, u);
            const double w = g.edges[e].weight;
            const double nd = dist[u] + w;
            if (done[v]) {
                // 一般情況直接略過；只有負權重邊才會發生「已確定的點還能變短」
                if (nd < dist[v]) {
                    rec.flashEdge(e, EdgeState::Rejected);
                    rec.step(7, std::format("經過 {} 到 {} 只要 {}，比確定的 {} 還短！但 {} 已經確定、不會再改了。"
                                            "這是負權重邊造成的：Dijkstra 在這裡算錯了（改用 Bellman-Ford）。",
                                            g.name(u), g.name(v), fmtNum(nd), fmtNum(dist[v]), g.name(v)));
                }
                continue;
            }
            rec.flashEdge(e, EdgeState::Examining);
            if (nd < dist[v]) {
                const double old = dist[v];
                if (parentEdge[v] >= 0) rec.edge(parentEdge[v], EdgeState::Normal);
                parentEdge[v] = e;
                dist[v] = nd;
                pq.push({nd, seq++, v});
                rec.edge(e, EdgeState::Tree);
                rec.node(v, NodeState::Frontier);
                rec.label(v, fmtNum(nd));
                showQueue();
                rec.step(8, std::format("經過 {} 到 {}：{} + {} = {}，比原本的 {} 短 → 更新，放進佇列。",
                                        g.name(u), g.name(v), fmtNum(dist[u]), fmtNum(w), fmtNum(nd), fmtNum(old)));
            } else {
                rec.step(7, std::format("經過 {} 到 {}：{} + {} = {}，沒有比原本的 {} 短，不更新。", g.name(u),
                                        g.name(v), fmtNum(dist[u]), fmtNum(w), fmtNum(nd), fmtNum(dist[v])));
            }
        }
        rec.node(u, NodeState::Visited);
    }

    res.dist = dist;
    if (dst < 0) {
        rec.step(1, "佇列空了，所有走得到的點的最短距離都確定了。藍色的邊就是「最短路徑樹」。");
    } else if (found && markPath(rec, g, parentEdge, src, dst, res)) {
        rec.step(9, std::format("最短路徑：{}，總距離 {}。", pathString(g, res.path), fmtNum(res.pathCost)));
    } else {
        rec.step(1, "佇列空了還沒碰到終點 → 終點走不到。");
    }
    return rec.finish(std::move(res));
}

// ---------------------------------------------------------------------------
// A*：Dijkstra 加上「離終點還有多遠」的估計 h，優先往終點方向找。
// 只要 h 不會高估（而且一致），找到的仍然是最短路徑。
// ---------------------------------------------------------------------------
Trace astar(const Graph& g, int src, int dst, const Heuristic& h) {
    assert(dst >= 0 && "A* 一定要有終點");
    Recorder rec(g, "A* 搜尋",
                 {
                     "g[起點] = 0，f = g + h，起點放進 open（依 f 排序）",
                     "while open 不是空的:",
                     "    u = 取出 f 最小的點",
                     "    if u 已在 closed: 跳過（過期的舊紀錄）",
                     "    把 u 放進 closed",
                     "    if u 是終點: 結束",
                     "    for 每條從 u 出發的邊 u→v（權重 w）:",
                     "        if g[u] + w < g[v]:",
                     "            g[v] = g[u] + w，f[v] = g[v] + h(v)，v 放進 open",
                     "沿著「從哪來」往回走，得到路徑",
                 },
                 "open（f 小的先出來）");
    const int n = g.size();
    std::vector<double> gs(n, INF);
    std::vector<int> parentEdge(n, -1);
    std::vector<bool> closed(n, false);
    Result res;

    struct Item {
        double f;
        double h;
        long long seq;
        int v;
    };
    // f 相同時先挑 h 小的（離終點近的），格子地圖上會走得比較直
    struct ItemLess {
        bool operator()(const Item& a, const Item& b) const {
            if (a.f != b.f) return a.f < b.f;
            if (a.h != b.h) return a.h < b.h;
            return a.seq < b.seq;
        }
    };
    MinHeap<Item, ItemLess> open;
    long long seq = 0;

    auto showOpen = [&] {
        std::vector<std::string> items;
        for (const Item& it : open.sorted()) {
            const bool stale = closed[it.v] || it.f - it.h > gs[it.v];
            items.push_back(std::format("{}：f={}（g={} + h={}）{}", g.name(it.v), fmtNum(it.f), fmtNum(it.f - it.h),
                                        fmtNum(it.h), stale ? "（過期）" : ""));
        }
        rec.container(std::move(items));
    };
    auto gh = [&](int v) { return std::format("{}+{}", fmtNum(gs[v]), fmtNum(h(v))); };

    gs[src] = 0;
    open.push({h(src), h(src), seq++, src});
    rec.node(src, NodeState::Frontier);
    rec.label(src, gh(src));
    showOpen();
    rec.step(0, std::format("g = 從起點走過來的實際距離，h = 估計還要多遠才到終點（直線距離，一定不會高估）。"
                            "A* 每次挑 f = g + h 最小的點，所以會優先往終點的方向找。點旁標的是 g+h。起點 h = {}。",
                            fmtNum(h(src))));

    bool found = false;
    while (!open.empty()) {
        const Item it = open.pop();
        const int u = it.v;
        if (closed[u]) {
            showOpen();
            rec.step(3, std::format("取出 {}，但它已經在 closed 裡了，這是舊紀錄，丟掉。", g.name(u)));
            continue;
        }
        closed[u] = true;
        rec.node(u, NodeState::Current);
        showOpen();
        rec.step(4, std::format("取出 f 最小的 {}：g={} + h={} = f={}。放進 closed，它的 g 確定了。", g.name(u),
                                fmtNum(gs[u]), fmtNum(h(u)), fmtNum(it.f)));
        if (u == dst) {
            found = true;
            rec.step(5, "終點進了 closed，結束。");
            break;
        }
        for (int e : g.adj[u]) {
            const int v = g.other(e, u);
            if (closed[v]) continue;
            const double w = g.edges[e].weight;
            const double ng = gs[u] + w;
            rec.flashEdge(e, EdgeState::Examining);
            if (ng < gs[v]) {
                const double old = gs[v];
                if (parentEdge[v] >= 0) rec.edge(parentEdge[v], EdgeState::Normal);
                parentEdge[v] = e;
                gs[v] = ng;
                open.push({ng + h(v), h(v), seq++, v});
                rec.edge(e, EdgeState::Tree);
                rec.node(v, NodeState::Frontier);
                rec.label(v, gh(v));
                showOpen();
                rec.step(8, std::format("經過 {} 到 {}：g = {} + {} = {}（原本 {}）→ 更新。h({}) = {}，f = {}。",
                                        g.name(u), g.name(v), fmtNum(gs[u]), fmtNum(w), fmtNum(ng), fmtNum(old),
                                        g.name(v), fmtNum(h(v)), fmtNum(ng + h(v))));
            } else {
                rec.step(7, std::format("經過 {} 到 {}：g = {} + {} = {}，沒有比原本的 {} 短，不更新。", g.name(u),
                                        g.name(v), fmtNum(gs[u]), fmtNum(w), fmtNum(ng), fmtNum(gs[v])));
            }
        }
        rec.node(u, NodeState::Visited);
    }

    res.dist = gs;
    if (found && markPath(rec, g, parentEdge, src, dst, res)) {
        int expanded = 0;
        for (bool c : closed) expanded += c ? 1 : 0;
        rec.step(9, std::format("最短路徑：{}，總距離 {}。只展開了 {} 個點（跟 Dijkstra 比比看）。",
                                pathString(g, res.path), fmtNum(res.pathCost), expanded));
    } else {
        rec.step(1, "open 空了還沒碰到終點 → 終點走不到。");
    }
    return rec.finish(std::move(res));
}

// ---------------------------------------------------------------------------
// Bellman-Ford：不挑順序，把每條邊都鬆弛 V-1 輪。慢，但允許負權重，
// 還能偵測負環。
// ---------------------------------------------------------------------------
Trace bellmanFord(const Graph& g, int src, int dst) {
    Recorder rec(g, "Bellman-Ford 最短路徑",
                 {
                     "dist[所有點] = ∞，dist[起點] = 0",
                     "重複 V - 1 輪:",
                     "    for 每條邊 u→v（權重 w）:",
                     "        if dist[u] + w < dist[v]: dist[v] = dist[u] + w",
                     "    這一輪完全沒更新 → 已經穩定，提早結束",
                     "再檢查一輪：如果還能更新 → 有負環",
                     "沿著「從哪來」往回走，得到路徑",
                 },
                 "進度");
    const int n = g.size();
    std::vector<double> dist(n, INF);
    std::vector<int> parentEdge(n, -1);
    Result res;

    // 無向邊當成兩個方向各一條
    struct Arc {
        int e, u, v;
    };
    std::vector<Arc> arcs;
    for (int e = 0; e < static_cast<int>(g.edges.size()); ++e) {
        const Edge& ed = g.edges[e];
        arcs.push_back({e, ed.from, ed.to});
        if (!g.directed) arcs.push_back({e, ed.to, ed.from});
    }

    for (int u = 0; u < n; ++u) rec.label(u, "∞");
    dist[src] = 0;
    rec.label(src, "0");
    rec.node(src, NodeState::Frontier);
    rec.step(0, std::format("起點距離 0，其他 ∞。最短路徑最多經過 V - 1 = {} 條邊，"
                            "而每一輪「把所有邊檢查一次」至少能多確定一條邊，所以最多 {} 輪就夠了。",
                            n - 1, n - 1));

    auto relax = [&](const Arc& a, double nd) {
        if (parentEdge[a.v] >= 0) rec.edge(parentEdge[a.v], EdgeState::Normal);
        parentEdge[a.v] = a.e;
        dist[a.v] = nd;
        rec.edge(a.e, EdgeState::Tree);
        rec.label(a.v, fmtNum(nd));
        rec.node(a.v, NodeState::Frontier);
        rec.flashNode(a.v, NodeState::Current);
    };

    bool stable = false;
    for (int round = 1; round <= n - 1; ++round) {
        int updates = 0;
        auto progress = [&] {
            rec.container({std::format("第 {} 輪／最多 {} 輪", round, n - 1), std::format("這一輪更新了 {} 次", updates)});
        };
        progress();
        rec.step(1, std::format("第 {} 輪開始：把每條邊依序檢查一次。", round));
        for (const Arc& a : arcs) {
            const double w = g.edges[a.e].weight;
            rec.flashEdge(a.e, EdgeState::Examining);
            if (dist[a.u] == INF) {
                rec.step(3, std::format("{}→{}：dist[{}] 還是 ∞，從它出發還沒有意義，跳過。", g.name(a.u),
                                        g.name(a.v), g.name(a.u)));
                continue;
            }
            const double nd = dist[a.u] + w;
            if (nd < dist[a.v]) {
                const double old = dist[a.v];
                relax(a, nd);
                ++updates;
                progress();
                rec.step(3, std::format("{}→{}：{} + ({}) = {}，比 dist[{}] = {} 小 → 更新。", g.name(a.u),
                                        g.name(a.v), fmtNum(dist[a.u]), fmtNum(w), fmtNum(nd), g.name(a.v),
                                        fmtNum(old)));
            } else {
                rec.step(3, std::format("{}→{}：{} + ({}) = {}，沒有比 {} 小，不更新。", g.name(a.u), g.name(a.v),
                                        fmtNum(dist[a.u]), fmtNum(w), fmtNum(nd), fmtNum(dist[a.v])));
            }
        }
        if (updates == 0) {
            stable = true;
            rec.step(4, std::format("第 {} 輪完全沒有更新：距離都穩定了，提早結束（也代表沒有負環）。", round));
            break;
        }
    }

    if (!stable) {
        for (const Arc& a : arcs) {
            const double nd = dist[a.u] + g.edges[a.e].weight;
            if (dist[a.u] == INF || nd >= dist[a.v]) continue;

            // 還能更新 → 有負環。沿著 parent 退 V 步一定會掉進環裡，再繞一圈把環找出來。
            res.negativeCycle = true;
            parentEdge[a.v] = a.e;
            auto parentOf = [&](int y) { return parentEdge[y] < 0 ? -1 : g.other(parentEdge[y], y); };
            int x = a.v;
            for (int i = 0; i < n && x >= 0; ++i) x = parentOf(x);
            std::string cycleText;
            if (x >= 0) {
                std::vector<int> cycle{x};
                for (int y = parentOf(x); y != x && y >= 0; y = parentOf(y)) cycle.push_back(y);
                double cycleWeight = 0;
                for (int y : cycle) {
                    cycleWeight += g.edges[parentEdge[y]].weight;
                    rec.edge(parentEdge[y], EdgeState::Path);
                    rec.node(y, NodeState::Path);
                }
                std::reverse(cycle.begin(), cycle.end());
                cycle.push_back(cycle.front());
                cycleText = std::format("{}，繞一圈權重 {}。", pathString(g, cycle), fmtNum(cycleWeight));
            }
            rec.flashEdge(a.e, EdgeState::Rejected);
            rec.step(5, std::format("做完 {} 輪後 {}→{} 居然還能變短 → 有負環！{}"
                                    "可以無限繞圈讓距離一直變小，所以「最短路徑」不存在。",
                                    n - 1, g.name(a.u), g.name(a.v), cycleText));
            break;
        }
        if (!res.negativeCycle) rec.step(5, std::format("做滿 {} 輪後再檢查一次，沒有任何邊能更新 → 沒有負環。", n - 1));
    }

    res.dist = dist;
    if (!res.negativeCycle) {
        for (int u = 0; u < n; ++u)
            if (dist[u] != INF) rec.node(u, NodeState::Visited);
        if (dst >= 0 && markPath(rec, g, parentEdge, src, dst, res))
            rec.step(6, std::format("最短路徑：{}，總距離 {}。", pathString(g, res.path), fmtNum(res.pathCost)));
        else if (dst >= 0)
            rec.step(6, "終點走不到。");
        else
            rec.step(6, "所有點的最短距離都算好了。");
    }
    return rec.finish(std::move(res));
}

// ---------------------------------------------------------------------------
// Floyd-Warshall：一次算出所有點對的最短距離。
// 第 k 輪的意思是「允許經過前 k 個點當中繼」。
// ---------------------------------------------------------------------------
Trace floydWarshall(const Graph& g, int src, int dst) {
    Recorder rec(g, "Floyd-Warshall 全點對最短路徑",
                 {
                     "d[i][j] = 邊的權重（沒有邊 = ∞，d[i][i] = 0）",
                     "for k = 每個點（允許經過 k 當中繼）:",
                     "    for 每一對 (i, j):",
                     "        if d[i][k] + d[k][j] < d[i][j]:",
                     "            d[i][j] = d[i][k] + d[k][j]",
                     "如果 d[i][i] < 0 → 有負環",
                     "用 next 表重建起點到終點的路徑",
                 },
                 "中繼點");
    const int n = g.size();
    std::vector<std::vector<double>> d(n, std::vector<double>(n, INF));
    std::vector<std::vector<int>> next(n, std::vector<int>(n, -1));  // i 到 j 的下一步
    Result res;

    rec.initMatrix(n);
    for (int i = 0; i < n; ++i) {
        d[i][i] = 0;
        next[i][i] = i;
    }
    for (const Edge& e : g.edges) {
        auto put = [&](int a, int b) {
            if (e.weight < d[a][b]) {
                d[a][b] = e.weight;
                next[a][b] = b;
            }
        };
        put(e.from, e.to);
        if (!g.directed) put(e.to, e.from);
    }
    for (int i = 0; i < n; ++i)
        for (int j = 0; j < n; ++j) rec.cell(i, j, fmtNum(d[i][j]));
    rec.step(0, "先把直接相連的邊填進矩陣：d[i][j] = 從 i 直接走到 j 的距離（第 i 列、第 j 行）。這時還不准經過任何中繼點。");

    std::string allowed;  // 之前已經允許的中繼點
    for (int k = 0; k < n; ++k) {
        rec.node(k, NodeState::Current);
        rec.container({std::format("現在的中繼點 k = {}", g.name(k)),
                       "之前允許的：" + (allowed.empty() ? std::string("（無）") : allowed)});
        rec.step(1, std::format("現在開始允許經過 {} 中轉。檢查每一對 i→j：繞道 {} 會不會比較近？", g.name(k),
                                g.name(k)));
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < n; ++j) {
                // i→k 或 k→j 走不到的組合不可能變短，直接略過不列成一步。
                // 注意 i == j 不能略過：d[i][i] 變成負的就是負環的證據。
                if (i == k || j == k || d[i][k] == INF || d[k][j] == INF) continue;
                const double cand = d[i][k] + d[k][j];
                if (i == j && cand >= d[i][j]) continue;  // 繞一圈回來沒變負，沒什麼好看的
                rec.flashMark(i, k, CellMark::Via);
                rec.flashMark(k, j, CellMark::Via);
                rec.flashMark(i, j, CellMark::Target);
                rec.flashNode(i, NodeState::Frontier);
                rec.flashNode(j, NodeState::Frontier);
                const std::string head = std::format("d[{}][{}]：繞道 {} = d[{}][{}] + d[{}][{}] = {} + {} = {}",
                                                     g.name(i), g.name(j), g.name(k), g.name(i), g.name(k), g.name(k),
                                                     g.name(j), fmtNum(d[i][k]), fmtNum(d[k][j]), fmtNum(cand));
                if (cand < d[i][j]) {
                    const double old = d[i][j];
                    d[i][j] = cand;
                    next[i][j] = next[i][k];
                    rec.cell(i, j, fmtNum(cand));
                    rec.flashMark(i, j, CellMark::Updated);
                    rec.step(4, std::format("{}，比原本的 {} 短 → 更新。", head, fmtNum(old)));
                } else {
                    rec.step(3, std::format("{}，沒有比原本的 {} 短，不更新。", head, fmtNum(d[i][j])));
                }
            }
        }
        rec.node(k, NodeState::Visited);
        allowed += (allowed.empty() ? "" : "、") + g.name(k);
    }

    res.allPairs = d;
    for (int i = 0; i < n; ++i) {
        if (d[i][i] < 0) {
            res.negativeCycle = true;
            rec.flashMark(i, i, CellMark::Target);
        }
    }
    if (res.negativeCycle) {
        rec.step(5, "對角線出現負數：從某個點出發繞一圈回來竟然是負的 → 有負環，這些距離都沒有意義。");
        return rec.finish(std::move(res));
    }

    if (src >= 0 && dst >= 0 && d[src][dst] != INF) {
        std::vector<int> path{src};
        for (int u = src; u != dst;) {
            u = next[u][dst];
            path.push_back(u);
        }
        for (std::size_t s = 0; s + 1 < path.size(); ++s) {
            // 兩點之間可能有多條邊，挑最輕的那條畫出來
            int best = -1;
            for (int e : g.adj[path[s]]) {
                if (g.other(e, path[s]) != path[s + 1]) continue;
                if (best < 0 || g.edges[e].weight < g.edges[best].weight) best = e;
            }
            if (best >= 0) rec.edge(best, EdgeState::Path);
        }
        for (int u : path) rec.node(u, NodeState::Path);
        res.path = path;
        res.pathCost = d[src][dst];
        rec.flashMark(src, dst, CellMark::Updated);
        rec.step(6, std::format("所有點對都算完了。以 {} → {} 為例：d = {}，路徑 {}。", g.name(src), g.name(dst),
                                fmtNum(d[src][dst]), pathString(g, path)));
    } else {
        rec.step(5, "所有點對都算完了，對角線沒有負數 → 沒有負環。");
    }
    return rec.finish(std::move(res));
}

}  // namespace gav
