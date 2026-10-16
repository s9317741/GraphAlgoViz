#include "core/tree.hpp"

#include <functional>
#include <string>

namespace gav {

namespace {

int addTreeNode(BinaryTree& t, std::string name) {
    const int id = t.graph.addNode(std::move(name), {});
    t.left.push_back(-1);
    t.right.push_back(-1);
    t.leftEdge.push_back(-1);
    t.rightEdge.push_back(-1);
    return id;
}

void link(BinaryTree& t, int parent, int child, bool isLeft) {
    const int e = t.graph.addEdge(parent, child, 1);
    (isLeft ? t.left : t.right)[parent] = child;
    (isLeft ? t.leftEdge : t.rightEdge)[parent] = e;
}

// 排版：x = 中序的名次（所以左子樹一定在左邊）、y = 深度
void layout(BinaryTree& t) {
    int rank = 0;
    std::function<void(int, int)> place = [&](int u, int depth) {
        if (u < 0) return;
        place(t.left[u], depth + 1);
        t.graph.nodes[u].pos = {float(rank++) * 80, float(depth) * 110};
        place(t.right[u], depth + 1);
    };
    place(t.root, 0);
}

}  // namespace

BinaryTree makeBst(const std::vector<int>& values) {
    BinaryTree t;
    std::vector<int> key;
    for (int v : values) {
        if (t.root < 0) {
            t.root = addTreeNode(t, std::to_string(v));
            key.push_back(v);
            continue;
        }
        int u = t.root;
        while (true) {
            if (v == key[u]) break;
            const bool goLeft = v < key[u];
            const int next = goLeft ? t.left[u] : t.right[u];
            if (next >= 0) {
                u = next;
                continue;
            }
            const int child = addTreeNode(t, std::to_string(v));
            key.push_back(v);
            link(t, u, child, goLeft);
            break;
        }
    }
    layout(t);
    return t;
}

BinaryTree makeExpressionTree() {
    BinaryTree t;
    const int times = addTreeNode(t, "×");
    const int plus = addTreeNode(t, "+");
    const int minus = addTreeNode(t, "-");
    const int n3 = addTreeNode(t, "3"), n4 = addTreeNode(t, "4"), n5 = addTreeNode(t, "5"), n2 = addTreeNode(t, "2");
    t.root = times;
    link(t, times, plus, true);
    link(t, times, minus, false);
    link(t, plus, n3, true);
    link(t, plus, n4, false);
    link(t, minus, n5, true);
    link(t, minus, n2, false);
    layout(t);
    return t;
}

}  // namespace gav
