#pragma once

// 範例場景與「哪個演算法配哪些場景」

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "core/graph.hpp"
#include "core/trace.hpp"
#include "core/tree.hpp"

namespace gav {

enum class Algo {
    BFS, DFS,                                    // 圖的走訪
    Dijkstra, AStar, BellmanFord, FloydWarshall, // 最短路徑
    Prim, Kruskal, TopoSort,                     // 其他圖論
    PreOrder, InOrder, PostOrder, LevelOrder,    // 樹的走訪
    Bubble, Selection, Insertion, Merge, Quick, Heap,  // 排序
};
inline constexpr int AlgoCount = 19;

struct AlgoInfo {
    const char* shortName;
    const char* idea;
    const char* complexity;
};
const AlgoInfo& algoInfo(Algo a);

// 畫面上方的分類
struct Category {
    const char* name;
    std::vector<Algo> algos;
};
const std::vector<Category>& categories();
int categoryOf(Algo a);

enum class SceneKind {
    City, Grid, Negative, NegativeCycle, Dag,                                  // 圖
    Bst, Expression,                                                           // 樹
    ArrayRandom, ArrayNearlySorted, ArrayReversed, ArrayFewUnique, ArrayLarge, // 陣列
};
inline constexpr int SceneCount = 12;

// 格子地圖：每一格是一個點，上下左右相連
struct GridMap {
    enum class Cell : std::uint8_t { Empty, Wall, Mud };
    static constexpr double MudCost = 5;  // 走進泥地的成本（一般格子是 1）

    int rows = 0;
    int cols = 0;
    std::vector<Cell> cells;

    int id(int r, int c) const { return r * cols + c; }
    Cell& at(int id) { return cells[id]; }
    Cell at(int id) const { return cells[id]; }

    // 格子編號就是點編號（牆也是點，只是沒有邊）
    Graph toGraph() const;

    void clear();
    void randomObstacles(unsigned seed, int keepA, int keepB);
    // 迷宮的通道在奇數座標上，所以起點終點會被移到最近的奇數格
    void maze(unsigned seed, int& start, int& goal);
};

struct Scene {
    SceneKind kind{};
    std::string name;
    std::string note;
    Graph graph;
    int start = -1;
    int goal = -1;
    std::optional<GridMap> grid;     // 有值時 graph 由它產生
    std::optional<BinaryTree> tree;  // 樹的場景：graph 是 tree->graph 的複本
    std::vector<int> array;          // 排序的場景
    bool reshufflable = false;       // 可以「重新產生」（隨機資料）
    double unit = 1;              // 座標距離 ÷ unit = 權重下限，A* 的直線距離估計也除以它
    bool showWeights = true;

    void rebuild();                  // 格子地圖改過後重建 graph
    double heuristic(int u) const;   // u 到 goal 的估計（不會高估）
};

Scene makeScene(SceneKind kind, unsigned seed = 1);  // seed 只影響隨機產生的場景
std::vector<SceneKind> scenesFor(Algo a);
Trace runAlgorithm(Algo a, const Scene& scene);

}  // namespace gav
