// 二元樹走訪：前序、中序、後序（遞迴）與層序（佇列）
//
// 前中後序的差別只在「輸出自己」放在走左右子樹的前面、中間還是後面。

#include <deque>
#include <format>
#include <functional>

#include "core/algorithms.hpp"

namespace gav {

namespace {

enum class Order { Pre, In, Post };

std::string joinNames(const Graph& g, const std::vector<int>& nodes) {
    std::string s;
    for (int u : nodes) s += (s.empty() ? "" : " ") + g.name(u);
    return s;
}

Trace depthFirst(const BinaryTree& t, Order order) {
    const Graph& g = t.graph;
    const char* fn = order == Order::Pre ? "preorder" : order == Order::In ? "inorder" : "postorder";
    const std::string self = "    輸出 node";
    const std::string left = std::format("    {}(node.左)", fn);
    const std::string right = std::format("    {}(node.右)", fn);
    std::vector<std::string> pseudo{std::format("{}(node):", fn), "    if node 是空的: return"};
    // 三種走訪唯一的差別：「輸出 node」這一行擺在哪
    if (order == Order::Pre) pseudo.insert(pseudo.end(), {self, left, right});
    if (order == Order::In) pseudo.insert(pseudo.end(), {left, self, right});
    if (order == Order::Post) pseudo.insert(pseudo.end(), {left, right, self});
    const int outLine = order == Order::Pre ? 2 : order == Order::In ? 3 : 4;
    const int leftLine = order == Order::Pre ? 3 : 2;
    const int rightLine = order == Order::Post ? 3 : 4;

    const char* title = order == Order::Pre ? "前序走訪（Preorder）" : order == Order::In ? "中序走訪（Inorder）" : "後序走訪（Postorder）";
    Recorder rec(g, title, pseudo, "輸出 ／ 呼叫堆疊");
    Result res;
    std::vector<int> stack;
    std::vector<bool> output(g.size(), false);

    auto show = [&] {
        std::vector<std::string> items{"輸出：" + (res.order.empty() ? std::string("（還沒有）") : joinNames(g, res.order))};
        for (auto it = stack.rbegin(); it != stack.rend(); ++it)
            items.push_back(std::format("{}{}({})", it == stack.rbegin() ? "→ " : "   ", fn, g.name(*it)));
        rec.container(std::move(items));
    };

    std::function<void(int)> visit = [&](int u) {
        stack.push_back(u);
        rec.node(u, NodeState::Current);
        show();
        rec.step(0, u == t.root ? std::format("從根 {} 開始呼叫 {}。", g.name(u), fn)
                                : std::format("進入 {}({})。", fn, g.name(u)));

        auto emit = [&] {
            output[u] = true;
            res.order.push_back(u);
            rec.label(u, std::format("#{}", res.order.size()));
            show();
            rec.step(outLine, std::format("輸出 {}（第 {} 個）。", g.name(u), res.order.size()));
        };
        auto descend = [&](bool isLeft) {
            const int child = isLeft ? t.left[u] : t.right[u];
            const int line = isLeft ? leftLine : rightLine;
            const char* side = isLeft ? "左" : "右";
            if (child < 0) {
                rec.step(line, std::format("{} 的{}邊是空的，呼叫下去馬上 return。", g.name(u), side));
                return;
            }
            const int e = isLeft ? t.leftEdge[u] : t.rightEdge[u];
            rec.edge(e, EdgeState::Tree);
            rec.node(u, output[u] ? NodeState::Visited : NodeState::Frontier);  // 先留在堆疊裡
            visit(child);
            rec.edge(e, EdgeState::Normal);
            rec.node(u, NodeState::Current);
            show();
            rec.step(line, std::format("{}子樹做完了，回到 {}。", side, g.name(u)));
        };

        if (order == Order::Pre) emit();
        descend(true);
        if (order == Order::In) emit();
        descend(false);
        if (order == Order::Post) emit();

        stack.pop_back();
        rec.node(u, NodeState::Visited);
    };

    if (t.root >= 0) visit(t.root);
    for (int u : res.order) rec.node(u, NodeState::Path);
    show();
    rec.step(0, std::format("走訪完成：{}", joinNames(g, res.order)));
    return rec.finish(std::move(res));
}

}  // namespace

Trace preorder(const BinaryTree& t) { return depthFirst(t, Order::Pre); }
Trace inorder(const BinaryTree& t) { return depthFirst(t, Order::In); }
Trace postorder(const BinaryTree& t) { return depthFirst(t, Order::Post); }

// 層序走訪就是樹上的 BFS：一層一層由左到右
Trace levelOrder(const BinaryTree& t) {
    const Graph& g = t.graph;
    Recorder rec(g, "層序走訪（Level-order）",
                 {
                     "根放進佇列",
                     "while 佇列不是空的:",
                     "    u = 從佇列取出，輸出 u",
                     "    if u 有左子節點: 放進佇列",
                     "    if u 有右子節點: 放進佇列",
                 },
                 "輸出 ／ 佇列");
    Result res;
    std::deque<int> queue;
    auto show = [&] {
        std::vector<std::string> items{"輸出：" + (res.order.empty() ? std::string("（還沒有）") : joinNames(g, res.order))};
        for (int v : queue) items.push_back("佇列：" + g.name(v));
        rec.container(std::move(items));
    };

    if (t.root >= 0) {
        queue.push_back(t.root);
        rec.node(t.root, NodeState::Frontier);
    }
    show();
    rec.step(0, "把根放進佇列。佇列先進先出，所以上面的層一定比下面的層先輸出。");
    while (!queue.empty()) {
        const int u = queue.front();
        queue.pop_front();
        res.order.push_back(u);
        rec.node(u, NodeState::Current);
        rec.label(u, std::format("#{}", res.order.size()));
        show();
        rec.step(2, std::format("取出 {} 並輸出（第 {} 個）。", g.name(u), res.order.size()));
        for (bool isLeft : {true, false}) {
            const int child = isLeft ? t.left[u] : t.right[u];
            const char* side = isLeft ? "左" : "右";
            if (child < 0) {
                rec.step(isLeft ? 3 : 4, std::format("{} 沒有{}子節點。", g.name(u), side));
                continue;
            }
            queue.push_back(child);
            rec.node(child, NodeState::Frontier);
            rec.flashEdge(isLeft ? t.leftEdge[u] : t.rightEdge[u], EdgeState::Tree);
            show();
            rec.step(isLeft ? 3 : 4, std::format("{}子節點 {} 放進佇列尾端。", side, g.name(child)));
        }
        rec.node(u, NodeState::Visited);
    }
    for (int u : res.order) rec.node(u, NodeState::Path);
    show();
    rec.step(1, std::format("佇列空了，走訪完成：{}", joinNames(g, res.order)));
    return rec.finish(std::move(res));
}

}  // namespace gav
