#include "core/scenes.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <format>
#include <random>

#include "core/algorithms.hpp"

namespace gav {

const AlgoInfo& algoInfo(Algo a) {
    static const std::array<AlgoInfo, AlgoCount> infos{{
        {"BFS", "從起點一層一層往外擴張。在沒有權重的圖上，第一次碰到終點就是邊數最少的路。", "時間 O(V + E)"},
        {"DFS", "一條路走到底，走不通再回頭（回溯）。適合判斷「有沒有路」、走迷宮、找連通塊。", "時間 O(V + E)"},
        {"Dijkstra", "每次挑目前距離最小的點，把它的距離定下來，再用它去更新鄰居。邊權重不能是負的。",
         "時間 O((V + E) log V)，用二元堆積"},
        {"A*", "Dijkstra 加上「離終點還有多遠」的估計 h，每次挑 g + h 最小的點，所以會朝終點方向找。",
         "最壞跟 Dijkstra 一樣，h 估得越準展開的點越少"},
        {"Bellman-Ford", "不挑順序，把每條邊重複鬆弛 V - 1 輪。比 Dijkstra 慢，但允許負權重，還能抓出負環。",
         "時間 O(V · E)"},
        {"Floyd-Warshall", "動態規劃：d[i][j] 依序允許經過第 1、2、…、k 個點中轉。一次算出所有點對的最短距離。",
         "時間 O(V³)，空間 O(V²)"},
        {"Prim", "從一個點長出一棵樹，每次加上「連到樹外」最便宜的那條邊。", "時間 O(E log V)"},
        {"Kruskal", "所有邊由小到大看，不會形成環就選。用並查集（Union-Find）判斷兩點是否已經連通。",
         "時間 O(E log E)，主要花在排序"},
        {"拓撲排序", "把有先後關係的事排順序：一直拿「沒有前置條件」的點出來。只適用有向無環圖（DAG）。",
         "時間 O(V + E)"},
        {"前序", "先輸出自己，再走左子樹、右子樹（根 → 左 → 右）。複製一棵樹、寫出前序表示法時用。",
         "時間 O(n)，遞迴深度 = 樹高"},
        {"中序", "先走左子樹，再輸出自己，最後右子樹（左 → 根 → 右）。二元搜尋樹的中序就是由小到大。",
         "時間 O(n)，遞迴深度 = 樹高"},
        {"後序", "左右子樹都走完才輸出自己（左 → 右 → 根）。子節點要先處理好的場合用，例如計算運算式、刪除整棵樹。",
         "時間 O(n)，遞迴深度 = 樹高"},
        {"層序", "一層一層由上到下、由左到右。其實就是樹上的 BFS，用佇列實作。", "時間 O(n)，佇列最多放最寬那一層"},
        {"泡沫排序", "相鄰兩個比較，順序錯就交換；每一輪最大的會浮到最右邊。", "時間 O(n²)；已排好時 O(n)（提早結束）· 穩定"},
        {"選擇排序", "每一輪從剩下的挑最小的放到最前面。比較次數固定，但交換最多只有 n-1 次。", "時間永遠 O(n²) · 不穩定"},
        {"插入排序", "像整理撲克牌，把每個數插進左邊已排好的部分。資料幾乎排好時非常快。",
         "時間 O(n²)；幾乎排好時接近 O(n) · 穩定"},
        {"合併排序", "切成兩半各自排好，再把兩個排好的序列合併成一個。", "時間永遠 O(n log n)，需要 O(n) 額外空間 · 穩定"},
        {"快速排序", "選一個基準，小的丟左邊、大的丟右邊，再對兩邊遞迴。實務上通常最快。",
         "平均 O(n log n)；最壞 O(n²)（基準老是選到最大或最小）· 不穩定"},
        {"堆積排序", "把陣列整理成最大堆，反覆把堆頂（最大值）換到最後面。", "時間永遠 O(n log n)，不需要額外空間 · 不穩定"},
    }};
    return infos[static_cast<int>(a)];
}

const std::vector<Category>& categories() {
    using enum Algo;
    static const std::vector<Category> list{
        {"圖的走訪", {BFS, DFS}},
        {"最短路徑", {Dijkstra, AStar, BellmanFord, FloydWarshall}},
        {"生成樹與排程", {Prim, Kruskal, TopoSort}},
        {"樹的走訪", {PreOrder, InOrder, PostOrder, LevelOrder}},
        {"排序", {Bubble, Selection, Insertion, Merge, Quick, Heap}},
    };
    return list;
}

int categoryOf(Algo a) {
    const auto& cats = categories();
    for (std::size_t c = 0; c < cats.size(); ++c)
        if (std::find(cats[c].algos.begin(), cats[c].algos.end(), a) != cats[c].algos.end()) return static_cast<int>(c);
    return 0;
}

std::vector<SceneKind> scenesFor(Algo a) {
    using enum SceneKind;
    switch (a) {
        case Algo::BFS:
        case Algo::DFS: return {City, Grid, Dag};
        case Algo::Dijkstra: return {City, Grid, Negative};
        case Algo::AStar: return {Grid, City};
        case Algo::BellmanFord: return {Negative, NegativeCycle, City};
        case Algo::FloydWarshall: return {Negative, NegativeCycle, City};
        case Algo::Prim:
        case Algo::Kruskal: return {City};
        case Algo::TopoSort: return {Dag};
        case Algo::PreOrder:
        case Algo::InOrder:
        case Algo::PostOrder:
        case Algo::LevelOrder: return {Bst, Expression};
        case Algo::Bubble:
        case Algo::Selection:
        case Algo::Insertion:
        case Algo::Merge:
        case Algo::Quick:
        case Algo::Heap: return {ArrayRandom, ArrayNearlySorted, ArrayReversed, ArrayFewUnique, ArrayLarge};
    }
    return {};
}

Trace runAlgorithm(Algo a, const Scene& s) {
    const Graph& g = s.graph;
    switch (a) {
        case Algo::BFS: return bfs(g, s.start, s.goal);
        case Algo::DFS: return dfs(g, s.start, s.goal);
        case Algo::Dijkstra: return dijkstra(g, s.start, s.goal);
        case Algo::AStar: return astar(g, s.start, s.goal, [&s](int u) { return s.heuristic(u); });
        case Algo::BellmanFord: return bellmanFord(g, s.start, s.goal);
        case Algo::FloydWarshall: return floydWarshall(g, s.start, s.goal);
        case Algo::Prim: return prim(g, s.start);
        case Algo::Kruskal: return kruskal(g);
        case Algo::TopoSort: return topologicalSort(g);
        case Algo::PreOrder: return preorder(*s.tree);
        case Algo::InOrder: return inorder(*s.tree);
        case Algo::PostOrder: return postorder(*s.tree);
        case Algo::LevelOrder: return levelOrder(*s.tree);
        case Algo::Bubble: return bubbleSort(s.array);
        case Algo::Selection: return selectionSort(s.array);
        case Algo::Insertion: return insertionSort(s.array);
        case Algo::Merge: return mergeSort(s.array);
        case Algo::Quick: return quickSort(s.array);
        case Algo::Heap: return heapSort(s.array);
    }
    return {};
}

// ---------------------------------------------------------------------------
// 格子地圖
// ---------------------------------------------------------------------------

Graph GridMap::toGraph() const {
    Graph g;
    g.directed = true;  // 走進泥地才要付成本，方向不同成本不同，所以是有向圖
    for (int r = 0; r < rows; ++r)
        for (int c = 0; c < cols; ++c) g.addNode(std::format("({},{})", r, c), {float(c), float(r)});

    constexpr int dr[] = {-1, 0, 1, 0};  // 上、右、下、左
    constexpr int dc[] = {0, 1, 0, -1};
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            if (at(id(r, c)) == Cell::Wall) continue;
            for (int k = 0; k < 4; ++k) {
                const int nr = r + dr[k], nc = c + dc[k];
                if (nr < 0 || nr >= rows || nc < 0 || nc >= cols) continue;
                const Cell to = at(id(nr, nc));
                if (to == Cell::Wall) continue;
                g.addEdge(id(r, c), id(nr, nc), to == Cell::Mud ? MudCost : 1);
            }
        }
    }
    return g;
}

void GridMap::clear() { std::fill(cells.begin(), cells.end(), Cell::Empty); }

void GridMap::randomObstacles(unsigned seed, int keepA, int keepB) {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<double> uni(0, 1);
    for (auto& cell : cells) {
        const double x = uni(rng);
        cell = x < 0.25 ? Cell::Wall : x < 0.37 ? Cell::Mud : Cell::Empty;
    }
    // 起點終點和它們旁邊留空，免得一開始就被關住
    for (int keep : {keepA, keepB}) {
        const int r = keep / cols, c = keep % cols;
        for (int y = std::max(0, r - 1); y <= std::min(rows - 1, r + 1); ++y)
            for (int x = std::max(0, c - 1); x <= std::min(cols - 1, c + 1); ++x) at(id(y, x)) = Cell::Empty;
    }
}

void GridMap::maze(unsigned seed, int& start, int& goal) {
    std::mt19937 rng(seed);
    std::fill(cells.begin(), cells.end(), Cell::Wall);

    // 遞迴回溯法（用明確的堆疊）：從一格出發隨機挖，挖不動就退回上一格
    std::vector<std::pair<int, int>> stack{{1, 1}};
    at(id(1, 1)) = Cell::Empty;
    while (!stack.empty()) {
        const auto [r, c] = stack.back();
        std::vector<std::pair<int, int>> next;
        constexpr std::array<std::pair<int, int>, 4> dirs{{{-2, 0}, {0, 2}, {2, 0}, {0, -2}}};
        for (auto [dr, dc] : dirs) {
            const int nr = r + dr, nc = c + dc;
            if (nr > 0 && nr < rows - 1 && nc > 0 && nc < cols - 1 && at(id(nr, nc)) == Cell::Wall)
                next.emplace_back(nr, nc);
        }
        if (next.empty()) {
            stack.pop_back();
            continue;
        }
        const auto [nr, nc] = next[std::uniform_int_distribution<std::size_t>(0, next.size() - 1)(rng)];
        at(id((r + nr) / 2, (c + nc) / 2)) = Cell::Empty;
        at(id(nr, nc)) = Cell::Empty;
        stack.emplace_back(nr, nc);
    }

    // 再隨機打通一些牆，讓迷宮有多條路（不然 BFS 和 DFS 找到的路會一樣）
    std::uniform_real_distribution<double> uni(0, 1);
    for (int r = 1; r < rows - 1; ++r)
        for (int c = 1; c < cols - 1; ++c)
            if (at(id(r, c)) == Cell::Wall && (r + c) % 2 == 1 && uni(rng) < 0.08) at(id(r, c)) = Cell::Empty;

    auto snap = [&](int cell) {
        const int r = std::clamp((cell / cols) | 1, 1, rows - 2);
        const int c = std::clamp((cell % cols) | 1, 1, cols - 2);
        return id(r, c);
    };
    start = snap(start);
    goal = snap(goal);
    if (start == goal) goal = id(rows - 2, cols - 2);
}

// ---------------------------------------------------------------------------
// 場景
// ---------------------------------------------------------------------------

void Scene::rebuild() {
    if (grid) graph = grid->toGraph();
}

double Scene::heuristic(int u) const {
    if (goal < 0) return 0;
    const Vec2 a = graph.nodes[u].pos, b = graph.nodes[goal].pos;
    // 格子只能上下左右走，用曼哈頓距離；一般圖用直線距離
    if (grid) return std::abs(a.x - b.x) + std::abs(a.y - b.y);
    return std::hypot(a.x - b.x, a.y - b.y) / unit;
}

namespace {

struct NodeSpec {
    const char* name;
    float x, y;
};
struct EdgeSpec {
    int from, to;
    double weight;  // 城市路網裡代表「比直線距離多出來的塞車成本」
};

Graph buildGraph(bool directed, std::initializer_list<NodeSpec> nodes, std::initializer_list<EdgeSpec> edges) {
    Graph g;
    g.directed = directed;
    for (const auto& n : nodes) g.addNode(n.name, {n.x, n.y});
    for (const auto& e : edges) g.addEdge(e.from, e.to, e.weight);
    return g;
}

Scene city() {
    Scene s;
    s.kind = SceneKind::City;
    s.name = "城市路網";
    s.note = "加權無向圖。權重大約跟距離成正比，有幾條路比較塞（權重較高）。左鍵點一個點設為起點、右鍵設為終點。";
    s.unit = 50;
    enum { A, B, C, D, E, F, G, H, I, J };
    s.graph = buildGraph(false,
                         {{"A", 80, 320}, {"B", 250, 130}, {"C", 240, 480}, {"D", 430, 300}, {"E", 450, 600},
                          {"F", 640, 140}, {"G", 660, 430}, {"H", 850, 270}, {"I", 860, 600}, {"J", 1050, 420}},
                         {{A, B, 0}, {A, C, 0}, {B, C, 1}, {B, D, 0}, {B, F, 3}, {C, D, 0}, {C, E, 0}, {D, F, 0},
                          {D, G, 0}, {E, G, 2}, {E, I, 0}, {F, H, 0}, {G, H, 0}, {G, I, 0}, {H, J, 2}, {I, J, 0}});
    // 權重 = 無條件進位(直線距離 / unit) + 塞車成本。
    // 所以權重一定 >= 直線距離 / unit，A* 用直線距離當估計就不會高估。
    for (Edge& e : s.graph.edges) {
        const Vec2 a = s.graph.nodes[e.from].pos, b = s.graph.nodes[e.to].pos;
        e.weight += std::ceil(std::hypot(a.x - b.x, a.y - b.y) / s.unit - 1e-9);
    }
    s.start = A;
    s.goal = J;
    return s;
}

Scene gridScene() {
    Scene s;
    s.kind = SceneKind::Grid;
    s.name = "格子地圖";
    s.note = "每格是一個點，上下左右相連。泥地走進去成本 5，一般格子 1。拖曳可畫牆／泥地，拖曳起點或終點可以移動它們。";
    GridMap m;
    m.rows = 25;
    m.cols = 41;
    m.cells.assign(m.rows * m.cols, GridMap::Cell::Empty);
    for (int r = 3; r <= 21; ++r) m.at(m.id(r, 20)) = GridMap::Cell::Wall;
    for (int r = 9; r <= 15; ++r)
        for (int c = 26; c <= 29; ++c) m.at(m.id(r, c)) = GridMap::Cell::Mud;
    s.start = m.id(12, 6);
    s.goal = m.id(12, 35);
    s.grid = std::move(m);
    s.showWeights = false;
    s.rebuild();
    return s;
}

// 負權重的例子。邊故意照「最不利」的順序列，Bellman-Ford 要跑滿 V - 1 輪才穩定。
Scene negative(bool withCycle) {
    Scene s;
    s.kind = withCycle ? SceneKind::NegativeCycle : SceneKind::Negative;
    s.name = withCycle ? "負環" : "負權重";
    s.note = withCycle ? "有向圖。C→D→E→C 繞一圈權重是 -3：越繞越短，最短路徑不存在。"
                       : "有向圖，含負權重邊。Dijkstra 在這裡會算錯（S 到 E 應該是 5），Bellman-Ford 不會。";
    enum { S, A, B, C, D, E };
    s.graph = buildGraph(true,
                         {{"S", 100, 330}, {"A", 330, 130}, {"B", 330, 530}, {"C", 600, 130}, {"D", 600, 530},
                          {"E", 860, 330}},
                         {{D, E, 3}, {C, E, 7}, {C, D, -2}, {A, C, 3}, {B, D, 6}, {B, A, -4}, {S, B, 5}, {S, A, 2}});
    if (withCycle) s.graph.addEdge(E, C, -4);
    s.start = S;
    s.goal = E;
    return s;
}

Scene dag() {
    Scene s;
    s.kind = SceneKind::Dag;
    s.name = "先修課程";
    s.note = "有向無環圖（DAG）：箭頭 X→Y 代表「修 Y 之前要先修 X」。";
    enum { Intro, Discrete, Linear, Prog, Arch, DS, OS, Alg, ML };
    s.graph = buildGraph(true,
                         {{"計概", 100, 120},
                          {"離散", 100, 420},
                          {"線代", 100, 620},
                          {"程設", 340, 120},
                          {"計組", 340, 330},
                          {"資結", 580, 220},
                          {"作系", 1060, 160},
                          {"演算", 820, 360},
                          {"機學", 1060, 520}},
                         {{Intro, Prog, 1}, {Intro, Arch, 1}, {Prog, DS, 1}, {Discrete, DS, 1}, {Discrete, Alg, 1},
                          {DS, Alg, 1}, {DS, OS, 1}, {Arch, OS, 1}, {Alg, ML, 1}, {Linear, ML, 1}});
    s.start = Intro;
    s.goal = ML;
    s.showWeights = false;
    return s;
}

Scene treeScene(SceneKind kind, unsigned seed) {
    Scene s;
    s.kind = kind;
    s.showWeights = false;
    if (kind == SceneKind::Bst) {
        s.name = "二元搜尋樹";
        s.note = "二元搜尋樹：左子樹都比自己小、右子樹都比自己大。看看哪一種走訪剛好會由小到大。";
        s.reshufflable = true;
        std::vector<int> values{50, 30, 70, 20, 40, 60, 80, 35, 45, 65};
        if (seed != 1) {
            // 1~99 洗牌後取前 11 個，依序插入
            std::vector<int> pool(99);
            for (int i = 0; i < 99; ++i) pool[i] = i + 1;
            std::shuffle(pool.begin(), pool.end(), std::mt19937(seed));
            values.assign(pool.begin(), pool.begin() + 11);
        }
        s.tree = makeBst(values);
    } else {
        s.name = "運算式樹";
        s.note = "葉子是數字、內部是運算子：(3 + 4) × (5 - 2)。前序 = 波蘭表示法、中序 = 平常的寫法（但少了括號）、"
                 "後序 = 逆波蘭表示法（計算機用一個堆疊就能直接算）。";
        s.tree = makeExpressionTree();
    }
    s.graph = s.tree->graph;
    return s;
}

Scene arrayScene(SceneKind kind, unsigned seed) {
    Scene s;
    s.kind = kind;
    s.reshufflable = kind != SceneKind::ArrayReversed;
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> value(5, 99);
    constexpr int N = 16;
    auto evenly = [] {
        std::vector<int> v(N);
        for (int i = 0; i < N; ++i) v[i] = 8 + i * 6;
        return v;
    };
    switch (kind) {
        case SceneKind::ArrayNearlySorted: {
            s.name = "幾乎排好";
            s.note = "只有少數幾個位置不對。插入排序在這裡特別快；快速排序（固定選最右邊當基準）反而會變慢。";
            s.array = evenly();
            for (int k = 0; k < 2; ++k) {
                const int i = std::uniform_int_distribution<int>(0, N - 3)(rng);
                std::swap(s.array[i], s.array[i + 2]);
            }
            break;
        }
        case SceneKind::ArrayReversed:
            s.name = "由大到小";
            s.note = "完全反過來，是很多排序演算法的最壞情況。";
            s.array = evenly();
            std::reverse(s.array.begin(), s.array.end());
            break;
        case SceneKind::ArrayFewUnique: {
            s.name = "很多重複";
            s.note = "只有 4 種值。觀察哪些排序會讓相等的值換來換去（不穩定）。";
            constexpr int kinds[] = {20, 45, 70, 95};
            for (int i = 0; i < N; ++i) s.array.push_back(kinds[std::uniform_int_distribution<int>(0, 3)(rng)]);
            break;
        }
        case SceneKind::ArrayLarge:
            s.name = "大量（60 個）";
            s.note = "60 個隨機數。比較 O(n²) 和 O(n log n)：看右下角的總步數和右側的比較次數。";
            for (int i = 0; i < 60; ++i) s.array.push_back(value(rng));
            break;
        default:
            s.name = "隨機";
            s.note = "16 個隨機數。";
            for (int i = 0; i < N; ++i) s.array.push_back(value(rng));
            break;
    }
    return s;
}

}  // namespace

Scene makeScene(SceneKind kind, unsigned seed) {
    switch (kind) {
        case SceneKind::City: return city();
        case SceneKind::Grid: return gridScene();
        case SceneKind::Negative: return negative(false);
        case SceneKind::NegativeCycle: return negative(true);
        case SceneKind::Dag: return dag();
        case SceneKind::Bst:
        case SceneKind::Expression: return treeScene(kind, seed);
        case SceneKind::ArrayRandom:
        case SceneKind::ArrayNearlySorted:
        case SceneKind::ArrayReversed:
        case SceneKind::ArrayFewUnique:
        case SceneKind::ArrayLarge: return arrayScene(kind, seed);
    }
    return {};
}

}  // namespace gav
