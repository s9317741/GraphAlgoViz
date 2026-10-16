// 最小生成樹：Prim 與 Kruskal

#include <algorithm>
#include <format>
#include <numeric>

#include "core/algo_common.hpp"
#include "core/algorithms.hpp"

namespace gav {

// ---------------------------------------------------------------------------
// Prim：從一個點開始長一棵樹，每次加「一端在樹裡、一端在樹外」最便宜的邊。
// ---------------------------------------------------------------------------
Trace prim(const Graph& g, int root) {
    Recorder rec(g, "Prim 最小生成樹",
                 {
                     "起點加入樹，把它的邊放進優先佇列",
                     "while 優先佇列不是空的:",
                     "    取出權重最小的邊 (u, v)，u 在樹裡",
                     "    if v 也已經在樹裡: 丟掉（加了會變成環）",
                     "    把 v 和這條邊加入樹",
                     "    把 v 連到樹外的邊放進佇列",
                     "佇列空了 → 完成",
                 },
                 "優先佇列（權重小的先出來）");
    const int n = g.size();
    std::vector<bool> inTree(n, false);
    Result res;

    struct Item {
        double w;
        long long seq;
        int e;
        int u;  // 在樹裡的那一端
    };
    struct ItemLess {
        bool operator()(const Item& a, const Item& b) const { return a.w != b.w ? a.w < b.w : a.seq < b.seq; }
    };
    MinHeap<Item, ItemLess> pq;
    long long seq = 0;

    auto showQueue = [&] {
        std::vector<std::string> items;
        for (const Item& it : pq.sorted()) {
            const int v = g.other(it.e, it.u);
            items.push_back(std::format("{}–{}：{}{}", g.name(it.u), g.name(v), fmtNum(it.w),
                                        inTree[v] ? "（兩端都在樹裡了）" : ""));
        }
        rec.container(std::move(items));
    };
    // v 加入樹：把通往樹外的邊都變成候選
    auto addToTree = [&](int v) {
        inTree[v] = true;
        rec.node(v, NodeState::Current);
        for (int e : g.adj[v]) {
            const int x = g.other(e, v);
            if (inTree[x]) continue;
            pq.push({g.edges[e].weight, seq++, e, v});
            rec.edge(e, EdgeState::Candidate);
        }
    };

    addToTree(root);
    showQueue();
    rec.step(0, std::format("從 {} 開始長一棵樹。虛線是候選邊：一端在樹裡、一端在樹外。", g.name(root)));

    int last = root;
    while (!pq.empty()) {
        const Item it = pq.pop();
        const int v = g.other(it.e, it.u);
        rec.node(last, NodeState::Visited);
        if (inTree[v]) {
            rec.edge(it.e, EdgeState::Rejected);
            showQueue();
            rec.step(3, std::format("取出 {}–{}（{}），但 {} 已經在樹裡了，加了會形成環 → 丟掉。", g.name(it.u),
                                    g.name(v), fmtNum(it.w), g.name(v)));
            continue;
        }
        rec.edge(it.e, EdgeState::Tree);
        res.treeEdges.push_back(it.e);
        res.treeWeight += it.w;
        addToTree(v);
        last = v;
        showQueue();
        rec.step(4, std::format("取出最便宜的 {}–{}（{}）。所有跨出樹的邊裡它最小，加進來一定不虧（切割性質）。"
                                "目前樹的總權重 = {}。",
                                g.name(it.u), g.name(v), fmtNum(it.w), fmtNum(res.treeWeight)));
    }
    rec.node(last, NodeState::Visited);

    std::string text = std::format("完成！最小生成樹共 {} 條邊，總權重 {}。", res.treeEdges.size(),
                                   fmtNum(res.treeWeight));
    if (static_cast<int>(res.treeEdges.size()) < n - 1) text += "（圖不連通，只涵蓋起點所在的那一塊）";
    rec.step(6, text);
    return rec.finish(std::move(res));
}

// ---------------------------------------------------------------------------
// Kruskal：所有邊由小到大看，不會成環就選。用 Union-Find 判斷兩點是否已連通。
// ---------------------------------------------------------------------------
Trace kruskal(const Graph& g) {
    Recorder rec(g, "Kruskal 最小生成樹",
                 {
                     "把所有邊依權重由小到大排好",
                     "for 每條邊 (u, v)，從小到大:",
                     "    if u 和 v 已經在同一群: 跳過（加了會變成環）",
                     "    選這條邊，把兩群合併成一群",
                     "選滿 V - 1 條邊 → 完成",
                 },
                 "還沒看的邊（由小到大）");
    const int n = g.size();
    Result res;

    // Union-Find（並查集）
    std::vector<int> parent(n), groupSize(n, 1);
    std::iota(parent.begin(), parent.end(), 0);
    auto find = [&](int x) {
        while (parent[x] != x) x = parent[x] = parent[parent[x]];
        return x;
    };

    std::vector<int> order(g.edges.size());
    std::iota(order.begin(), order.end(), 0);
    std::stable_sort(order.begin(), order.end(),
                     [&](int a, int b) { return g.edges[a].weight < g.edges[b].weight; });

    auto showRemaining = [&](std::size_t from) {
        std::vector<std::string> items;
        for (std::size_t i = from; i < order.size(); ++i) {
            const Edge& e = g.edges[order[i]];
            items.push_back(std::format("{}–{}：{}", g.name(e.from), g.name(e.to), fmtNum(e.weight)));
        }
        rec.container(std::move(items));
    };
    auto showGroups = [&] {
        for (int u = 0; u < n; ++u) rec.label(u, g.name(find(u)) + " 群");
    };

    showGroups();
    showRemaining(0);
    rec.step(0, "先把邊照權重排好。一開始每個點自成一群（點旁寫的是它屬於哪一群）。");

    bool complete = n <= 1;
    for (std::size_t i = 0; i < order.size() && !complete; ++i) {
        const int e = order[i];
        const Edge& ed = g.edges[e];
        const int ru = find(ed.from);
        const int rv = find(ed.to);
        rec.flashEdge(e, EdgeState::Examining);
        rec.flashNode(ed.from, NodeState::Current);
        rec.flashNode(ed.to, NodeState::Current);
        showRemaining(i + 1);
        const std::string head = std::format("{}–{}（{}）", g.name(ed.from), g.name(ed.to), fmtNum(ed.weight));
        if (ru == rv) {
            rec.edge(e, EdgeState::Rejected);
            rec.step(2, std::format("{}：{} 和 {} 已經在同一群（{} 群），加了會形成環 → 跳過。", head,
                                    g.name(ed.from), g.name(ed.to), g.name(ru)));
            continue;
        }
        // 小的群併進大的群（union by size），樹才不會太高
        const auto [big, small] = groupSize[ru] >= groupSize[rv] ? std::pair{ru, rv} : std::pair{rv, ru};
        parent[small] = big;
        groupSize[big] += groupSize[small];
        res.treeEdges.push_back(e);
        res.treeWeight += ed.weight;
        rec.edge(e, EdgeState::Tree);
        rec.node(ed.from, NodeState::Visited);
        rec.node(ed.to, NodeState::Visited);
        showGroups();
        rec.step(3, std::format("{}：兩端在不同群，選它！{} 群併進 {} 群。目前總權重 {}。", head, g.name(small),
                                g.name(big), fmtNum(res.treeWeight)));
        complete = static_cast<int>(res.treeEdges.size()) == n - 1;
    }

    if (complete)
        rec.step(4, std::format("選滿 V - 1 = {} 條邊，所有點連成一棵樹。總權重 {}。", n - 1, fmtNum(res.treeWeight)));
    else
        rec.step(4, std::format("邊都看完了但只選到 {} 條：圖不連通，得到的是最小生成森林，總權重 {}。",
                                res.treeEdges.size(), fmtNum(res.treeWeight)));
    return rec.finish(std::move(res));
}

}  // namespace gav
