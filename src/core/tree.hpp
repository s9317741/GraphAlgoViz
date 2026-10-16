#pragma once

#include <vector>

#include "core/graph.hpp"

namespace gav {

// 二元樹。graph 是畫圖用的（點 = 節點、邊 = 父子關係），
// left/right 才是走訪真正用到的結構。
struct BinaryTree {
    Graph graph;
    int root = -1;
    std::vector<int> left, right;          // 子節點編號，-1 = 沒有
    std::vector<int> leftEdge, rightEdge;  // 連到子節點的那條邊在 graph 裡的編號
};

// 依序插入建一棵二元搜尋樹（重複的值略過）
BinaryTree makeBst(const std::vector<int>& values);
// 運算式 (3 + 4) × (5 - 2)
BinaryTree makeExpressionTree();

}  // namespace gav
