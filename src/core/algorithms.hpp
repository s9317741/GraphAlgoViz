#pragma once

#include <functional>

#include "core/graph.hpp"
#include "core/trace.hpp"
#include "core/tree.hpp"

namespace gav {

// 估計 u 到終點還要多遠。A* 要求它「不會高估」才保證找到最短路徑。
using Heuristic = std::function<double(int u)>;

// dst < 0 表示沒有終點，走完所有走得到的點
Trace bfs(const Graph& g, int src, int dst);
Trace dfs(const Graph& g, int src, int dst);

Trace dijkstra(const Graph& g, int src, int dst);
Trace astar(const Graph& g, int src, int dst, const Heuristic& h);
Trace bellmanFord(const Graph& g, int src, int dst);
Trace floydWarshall(const Graph& g, int src, int dst);  // src/dst 只用來在最後畫出一條路徑

Trace prim(const Graph& g, int root);
Trace kruskal(const Graph& g);
Trace topologicalSort(const Graph& g);

// 二元樹走訪
Trace preorder(const BinaryTree& t);
Trace inorder(const BinaryTree& t);
Trace postorder(const BinaryTree& t);
Trace levelOrder(const BinaryTree& t);

// 排序（由小到大）
Trace bubbleSort(std::vector<int> a);
Trace selectionSort(std::vector<int> a);
Trace insertionSort(std::vector<int> a);
Trace mergeSort(std::vector<int> a);
Trace quickSort(std::vector<int> a);
Trace heapSort(std::vector<int> a);

}  // namespace gav
