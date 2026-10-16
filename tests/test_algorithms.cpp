// 演算法正確性測試。不需要開視窗，直接檢查 Trace 的結果與重播後的畫面狀態。
//
// 方法：同一個問題用不同演算法算，答案必須一致
// （Dijkstra = Bellman-Ford = Floyd-Warshall、A* = Dijkstra、Prim = Kruskal）。

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <random>
#include <vector>

#include "core/algorithms.hpp"
#include "core/scenes.hpp"

using namespace gav;

namespace {

int g_checks = 0;
int g_failures = 0;

#define CHECK(cond)                                                                \
    do {                                                                           \
        ++g_checks;                                                                \
        if (!(cond)) {                                                             \
            ++g_failures;                                                          \
            std::printf("  %s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #cond); \
        }                                                                          \
    } while (0)

struct TestCase {
    const char* name;
    void (*fn)();
};
std::vector<TestCase>& registry() {
    static std::vector<TestCase> tests;
    return tests;
}
#define TEST(name)                                                                       \
    void name();                                                                         \
    const bool name##_registered = (registry().push_back({#name, name}), true);          \
    void name()

bool same(double a, double b) { return a == b || std::abs(a - b) < 1e-9; }

bool sameDist(const std::vector<double>& a, const std::vector<double>& b) {
    if (a.size() != b.size()) return false;
    for (std::size_t i = 0; i < a.size(); ++i)
        if (!same(a[i], b[i])) return false;
    return true;
}

// 路徑上相鄰兩點真的有邊相連，而且權重加起來等於宣稱的成本
bool validPath(const Graph& g, const std::vector<int>& path, int src, int dst, double cost) {
    if (path.empty() || path.front() != src || path.back() != dst) return false;
    double sum = 0;
    for (std::size_t i = 0; i + 1 < path.size(); ++i) {
        double best = INF;
        for (int e : g.adj[path[i]])
            if (g.other(e, path[i]) == path[i + 1]) best = std::min(best, g.edges[e].weight);
        if (best == INF) return false;
        sum += best;
    }
    return same(sum, cost);
}

// 隨機圖。權重 ≥ 座標直線距離，所以直線距離可以當 A* 的估計（不會高估）。
Graph randomGraph(std::mt19937& rng, bool directed, int n, double density) {
    Graph g;
    g.directed = directed;
    std::uniform_real_distribution<float> coord(0, 100);
    std::uniform_real_distribution<double> uni(0, 1);
    std::uniform_int_distribution<int> extra(0, 5);
    for (int i = 0; i < n; ++i) g.addNode(std::string(1, char('A' + i)), {coord(rng), coord(rng)});
    for (int u = 0; u < n; ++u) {
        for (int v = directed ? 0 : u + 1; v < n; ++v) {
            if (u == v || uni(rng) > density) continue;
            const Vec2 a = g.nodes[u].pos, b = g.nodes[v].pos;
            g.addEdge(u, v, std::ceil(std::hypot(a.x - b.x, a.y - b.y)) + extra(rng));
        }
    }
    return g;
}

double straightLine(const Graph& g, int u, int v) {
    const Vec2 a = g.nodes[u].pos, b = g.nodes[v].pos;
    return std::hypot(a.x - b.x, a.y - b.y);
}

int reachableCount(const Graph& g, int src) {
    const Trace t = bfs(g, src, -1);
    return static_cast<int>(t.result.order.size());
}

}  // namespace

TEST(dijkstra_bellmanford_floyd_agree) {
    std::mt19937 rng(1);
    for (int iter = 0; iter < 300; ++iter) {
        const Graph g = randomGraph(rng, iter % 2 == 0, 2 + iter % 8, 0.4);
        const Trace fw = floydWarshall(g, -1, -1);
        for (int s = 0; s < g.size(); ++s) {
            const auto d1 = dijkstra(g, s, -1).result.dist;
            const auto d2 = bellmanFord(g, s, -1).result.dist;
            CHECK(sameDist(d1, d2));
            CHECK(sameDist(d1, fw.result.allPairs[s]));
        }
    }
}

TEST(astar_matches_dijkstra) {
    std::mt19937 rng(2);
    for (int iter = 0; iter < 300; ++iter) {
        const Graph g = randomGraph(rng, iter % 2 == 0, 2 + iter % 9, 0.35);
        const int src = static_cast<int>(rng() % g.size());
        const int dst = static_cast<int>(rng() % g.size());
        const double expected = dijkstra(g, src, -1).result.dist[dst];

        for (const Heuristic& h : {Heuristic([](int) { return 0.0; }),
                                   Heuristic([&](int u) { return straightLine(g, u, dst); })}) {
            const Result r = astar(g, src, dst, h).result;
            CHECK(same(r.pathCost, expected));
            if (expected != INF) CHECK(validPath(g, r.path, src, dst, r.pathCost));
            else CHECK(r.path.empty());
        }
        const Result d = dijkstra(g, src, dst).result;
        if (expected != INF) CHECK(validPath(g, d.path, src, dst, expected));
    }
}

TEST(bfs_finds_fewest_edges) {
    std::mt19937 rng(3);
    for (int iter = 0; iter < 200; ++iter) {
        Graph g = randomGraph(rng, iter % 2 == 0, 2 + iter % 9, 0.3);
        Graph unit = g;
        for (Edge& e : unit.edges) e.weight = 1;
        const int src = static_cast<int>(rng() % g.size());
        const auto levels = bfs(g, src, -1).result.dist;
        CHECK(sameDist(levels, dijkstra(unit, src, -1).result.dist));
    }
}

TEST(dfs_visits_everything_reachable) {
    std::mt19937 rng(4);
    for (int iter = 0; iter < 200; ++iter) {
        const Graph g = randomGraph(rng, iter % 2 == 0, 2 + iter % 9, 0.25);
        const int src = static_cast<int>(rng() % g.size());
        const Trace t = dfs(g, src, -1);
        CHECK(static_cast<int>(t.result.order.size()) == reachableCount(g, src));

        const int dst = static_cast<int>(rng() % g.size());
        const Result r = dfs(g, src, dst).result;
        if (dijkstra(g, src, -1).result.dist[dst] != INF) CHECK(validPath(g, r.path, src, dst, r.pathCost));
        else CHECK(r.path.empty());
    }
}

TEST(negative_weights) {
    const Scene s = makeScene(SceneKind::Negative);
    const Result bf = bellmanFord(s.graph, s.start, s.goal).result;
    CHECK(!bf.negativeCycle);
    CHECK(sameDist(bf.dist, {0, 1, 5, 4, 2, 5}));  // S A B C D E
    CHECK(same(bf.pathCost, 5));
    CHECK(validPath(s.graph, bf.path, s.start, s.goal, 5));
    CHECK(sameDist(floydWarshall(s.graph, -1, -1).result.allPairs[s.start], bf.dist));
    // 刻意示範的失敗案例：Dijkstra 遇到負權重會算錯
    CHECK(same(dijkstra(s.graph, s.start, s.goal).result.pathCost, 6));
}

TEST(negative_cycle_detected) {
    const Scene s = makeScene(SceneKind::NegativeCycle);
    CHECK(bellmanFord(s.graph, s.start, s.goal).result.negativeCycle);
    CHECK(floydWarshall(s.graph, s.start, s.goal).result.negativeCycle);
    CHECK(!bellmanFord(makeScene(SceneKind::Negative).graph, 0, -1).result.negativeCycle);
}

TEST(prim_equals_kruskal) {
    std::mt19937 rng(5);
    for (int iter = 0; iter < 300; ++iter) {
        const Graph g = randomGraph(rng, false, 2 + iter % 9, 0.5);
        const Result k = kruskal(g).result;
        const Result p = prim(g, 0).result;
        if (reachableCount(g, 0) == g.size()) {
            CHECK(same(k.treeWeight, p.treeWeight));
            CHECK(static_cast<int>(k.treeEdges.size()) == g.size() - 1);
            CHECK(static_cast<int>(p.treeEdges.size()) == g.size() - 1);
        } else {
            CHECK(static_cast<int>(k.treeEdges.size()) < g.size() - 1);
        }
    }
    const Scene city = makeScene(SceneKind::City);
    CHECK(same(prim(city.graph, 0).result.treeWeight, kruskal(city.graph).result.treeWeight));
}

TEST(topological_sort) {
    const Scene s = makeScene(SceneKind::Dag);
    const Result r = topologicalSort(s.graph).result;
    CHECK(!r.hasCycle);
    CHECK(static_cast<int>(r.order.size()) == s.graph.size());
    std::vector<int> position(s.graph.size());
    for (std::size_t i = 0; i < r.order.size(); ++i) position[r.order[i]] = static_cast<int>(i);
    for (const Edge& e : s.graph.edges) CHECK(position[e.from] < position[e.to]);

    Graph cyclic = s.graph;
    cyclic.addEdge(8, 0, 1);  // 機學 → 計概：形成環
    CHECK(topologicalSort(cyclic).result.hasCycle);
}

TEST(grid_scenes) {
    Scene s = makeScene(SceneKind::Grid);
    const double dij = dijkstra(s.graph, s.start, s.goal).result.pathCost;
    CHECK(dij != INF);
    CHECK(same(runAlgorithm(Algo::AStar, s).result.pathCost, dij));
    // 有泥地時 BFS 找的是格數最少，不一定最便宜
    CHECK(runAlgorithm(Algo::BFS, s).result.pathCost >= dij);

    s.grid->clear();
    s.rebuild();
    const double plain = dijkstra(s.graph, s.start, s.goal).result.pathCost;
    CHECK(same(runAlgorithm(Algo::BFS, s).result.pathCost, plain));
    CHECK(same(runAlgorithm(Algo::AStar, s).result.pathCost, plain));

    for (unsigned seed = 1; seed <= 20; ++seed) {
        s.grid->maze(seed, s.start, s.goal);
        s.rebuild();
        // 迷宮的通道是連通的，一定走得到
        const double m = dijkstra(s.graph, s.start, s.goal).result.pathCost;
        CHECK(m != INF);
        CHECK(same(runAlgorithm(Algo::AStar, s).result.pathCost, m));

        s.grid->randomObstacles(seed, s.start, s.goal);
        s.rebuild();
        const double o = dijkstra(s.graph, s.start, s.goal).result.pathCost;
        CHECK(same(runAlgorithm(Algo::AStar, s).result.pathCost, o));
    }
}

// 每個演算法 × 每個能用的場景：trace 要能完整重播，倒退再前進要得到同樣的畫面
TEST(traces_replay_consistently) {
    for (int a = 0; a < AlgoCount; ++a) {
        for (SceneKind kind : scenesFor(static_cast<Algo>(a))) {
            const Scene s = makeScene(kind);
            const Trace t = runAlgorithm(static_cast<Algo>(a), s);
            CHECK(!t.steps.empty());
            for (const Step& step : t.steps) {
                CHECK(step.line >= 0 && step.line < static_cast<int>(t.pseudocode.size()));
                CHECK(!step.text.empty());
            }

            Player p;
            p.load(&t);
            p.seek(p.count());
            const VisualState end = p.state();
            for (int u : t.result.path) CHECK(end.nodes[u] == NodeState::Path);

            p.seek(p.count() / 2);
            p.seek(p.count());
            CHECK(p.state().nodes == end.nodes);
            CHECK(p.state().edges == end.edges);
            CHECK(p.state().nodeLabels == end.nodeLabels);
            CHECK(p.state().matrix == end.matrix);
        }
    }
}

TEST(recorder_flash_lasts_one_step) {
    Graph g;
    g.addNode("A", {});
    g.addNode("B", {});
    g.addEdge(0, 1, 1);
    Recorder rec(g, "t", {"x"});
    rec.node(0, NodeState::Visited);
    rec.flashNode(0, NodeState::Current);
    rec.flashEdge(0, EdgeState::Examining);
    rec.step(0, "1");
    rec.step(0, "2");
    rec.flashNode(1, NodeState::Current);
    rec.node(1, NodeState::Frontier);  // 明確設定的狀態不會被 flash 還原
    rec.step(0, "3");
    rec.step(0, "4");
    const Trace t = rec.finish({});

    Player p;
    p.load(&t);
    p.seek(1);
    CHECK(p.state().nodes[0] == NodeState::Current);
    CHECK(p.state().edges[0] == EdgeState::Examining);
    p.seek(2);
    CHECK(p.state().nodes[0] == NodeState::Visited);
    CHECK(p.state().edges[0] == EdgeState::Normal);
    p.seek(4);
    CHECK(p.state().nodes[1] == NodeState::Frontier);
}

std::string names(const Graph& g, const std::vector<int>& order) {
    std::string s;
    for (int u : order) s += (s.empty() ? "" : " ") + g.name(u);
    return s;
}

TEST(tree_traversals) {
    const Scene bst = makeScene(SceneKind::Bst);
    auto run = [](Algo a, const Scene& s) { return names(s.graph, runAlgorithm(a, s).result.order); };
    CHECK(run(Algo::PreOrder, bst) == "50 30 20 40 35 45 70 60 65 80");
    CHECK(run(Algo::InOrder, bst) == "20 30 35 40 45 50 60 65 70 80");
    CHECK(run(Algo::PostOrder, bst) == "20 35 45 40 30 65 60 80 70 50");
    CHECK(run(Algo::LevelOrder, bst) == "50 30 70 20 40 60 80 35 45 65");

    const Scene expr = makeScene(SceneKind::Expression);
    CHECK(run(Algo::PreOrder, expr) == "× + 3 4 - 5 2");
    CHECK(run(Algo::InOrder, expr) == "3 + 4 × 5 - 2");
    CHECK(run(Algo::PostOrder, expr) == "3 4 + 5 2 - ×");
    CHECK(run(Algo::LevelOrder, expr) == "× + - 3 4 5 2");

    // 隨機二元搜尋樹：中序一定嚴格遞增，四種走訪都要拜訪每個點剛好一次
    for (unsigned seed = 2; seed <= 40; ++seed) {
        const Scene s = makeScene(SceneKind::Bst, seed);
        const auto in = runAlgorithm(Algo::InOrder, s).result.order;
        CHECK(static_cast<int>(in.size()) == s.graph.size());
        for (std::size_t i = 1; i < in.size(); ++i) CHECK(std::stoi(s.graph.name(in[i - 1])) < std::stoi(s.graph.name(in[i])));
        for (Algo a : {Algo::PreOrder, Algo::PostOrder, Algo::LevelOrder}) {
            auto order = runAlgorithm(a, s).result.order;
            std::sort(order.begin(), order.end());
            CHECK(static_cast<int>(std::unique(order.begin(), order.end()) - order.begin()) == s.graph.size());
        }
    }
}

TEST(sorts_produce_sorted_output) {
    const Algo sorts[] = {Algo::Bubble, Algo::Selection, Algo::Insertion, Algo::Merge, Algo::Quick, Algo::Heap};
    for (Algo a : sorts) {
        for (SceneKind kind : scenesFor(a)) {
            for (unsigned seed = 1; seed <= 15; ++seed) {
                const Scene s = makeScene(kind, seed);
                std::vector<int> expected = s.array;
                std::sort(expected.begin(), expected.end());
                const Trace t = runAlgorithm(a, s);
                CHECK(t.result.array == expected);

                // 畫面重播到最後也必須是排好的，而且每根都標成「已定位」
                Player p;
                p.load(&t);
                p.seek(p.count());
                CHECK(p.state().array == expected);
                CHECK(std::all_of(p.state().bars.begin(), p.state().bars.end(),
                                  [](BarState b) { return b == BarState::Sorted; }));
            }
        }
        for (const std::vector<int>& edge : {std::vector<int>{}, {7}, {2, 1}, {3, 3, 3}, {5, 4, 3, 2, 1}}) {
            std::vector<int> expected = edge;
            std::sort(expected.begin(), expected.end());
            Scene s;
            s.array = edge;
            CHECK(runAlgorithm(a, s).result.array == expected);
        }
    }
}

int main() {
    for (const TestCase& t : registry()) {
        const int before = g_failures;
        t.fn();
        std::printf("%s %s\n", g_failures == before ? "[ OK ]" : "[FAIL]", t.name);
    }
    std::printf("\n%d checks, %d failures\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
