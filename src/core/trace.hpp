#pragma once

// 演算法動畫的資料模型。
//
// 演算法本身不碰畫面：它一邊跑，一邊透過 Recorder 記下「這一步畫面哪裡變了、
// 為什麼變、對應虛擬碼第幾行」，最後得到一份 Trace。播放器再把 Trace 一步步
// 套到 VisualState 上畫出來。好處是可以倒退、拖進度條，也可以不開視窗直接測試。

#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "core/graph.hpp"

namespace gav {

inline constexpr double INF = std::numeric_limits<double>::infinity();

enum class NodeState : std::uint8_t {
    Unvisited,  // 還沒碰到
    Frontier,   // 已發現、等著處理（在佇列／堆疊裡）
    Current,    // 這一步正在處理
    Visited,    // 處理完了
    Path,       // 最後答案的一部分
};

enum class EdgeState : std::uint8_t {
    Normal,
    Candidate,  // 候選（例如 Prim 優先佇列裡的邊）
    Examining,  // 這一步正在檢查
    Tree,       // 目前的最短路徑樹／生成樹／已處理
    Rejected,   // 被捨棄
    Path,       // 最後答案
};

// Floyd-Warshall 距離矩陣格子的標示
enum class CellMark : std::uint8_t {
    None,
    Via,      // d[i][k]、d[k][j]：拿來組合的兩段
    Target,   // d[i][j]：正在比較的格子
    Updated,  // 這一步被改小了
};

// 排序時每根長條的狀態
enum class BarState : std::uint8_t {
    Normal,
    Active,   // 目前處理的範圍（例如合併排序正在合併的那一段）
    Compare,  // 正在比較
    Swap,     // 正在交換／寫入
    Pivot,    // 快速排序的基準、選擇排序目前的最小值
    Sorted,   // 已經在最終位置
};

// 長條下方的指標文字，例如 (3, "i")
using Pointer = std::pair<int, std::string>;

// 某一瞬間畫面該有的樣子
struct VisualState {
    std::vector<NodeState> nodes;
    std::vector<std::string> nodeLabels;
    std::vector<EdgeState> edges;
    int matrixSize = 0;  // 只有 Floyd-Warshall 會用到距離矩陣
    std::vector<std::string> matrix;
    std::vector<CellMark> marks;
    std::vector<int> array;  // 排序用的陣列，其他演算法為空
    std::vector<BarState> bars;
    std::vector<Pointer> pointers;
    std::vector<std::string> container;  // 佇列／堆疊等資料結構的內容
    std::string text;                    // 這一步的說明
    int line = -1;                       // 虛擬碼目前執行到第幾行
};

// 一步 = 相對於上一步的變化 + 說明。只存變化，倒退時從頭重播。
struct Step {
    std::vector<std::pair<int, NodeState>> nodes;
    std::vector<std::pair<int, std::string>> nodeLabels;
    std::vector<std::pair<int, EdgeState>> edges;
    std::vector<std::pair<int, std::string>> cells;
    std::vector<std::pair<int, CellMark>> marks;
    std::vector<std::pair<int, int>> values;
    std::vector<std::pair<int, BarState>> bars;
    std::optional<std::vector<Pointer>> pointers;
    std::optional<std::vector<std::string>> container;
    std::string text;
    int line = -1;
};

// 演算法的計算結果（給測試與結尾畫面用）
struct Result {
    std::vector<double> dist;
    std::vector<int> path;       // 起點到終點經過的點；走不到則為空
    double pathCost = INF;
    std::vector<int> order;      // 拜訪順序（BFS/DFS）或拓撲排序結果
    std::vector<int> treeEdges;  // 生成樹選中的邊
    double treeWeight = 0;
    std::vector<std::vector<double>> allPairs;  // Floyd-Warshall
    bool negativeCycle = false;
    bool hasCycle = false;  // 拓撲排序：圖中有環
    std::vector<int> array;  // 排序結果
};

struct Trace {
    std::string algorithm;
    std::vector<std::string> pseudocode;
    std::string containerTitle;
    VisualState initial;
    std::vector<Step> steps;
    Result result;
};

void applyStep(VisualState& state, const Step& step);

// 依序播放 Trace
class Player {
public:
    void load(const Trace* trace);
    void seek(int index);  // index = 已經套用了幾步（0 = 還沒開始）
    int index() const { return index_; }
    int count() const;
    const VisualState& state() const { return state_; }

private:
    const Trace* trace_ = nullptr;
    VisualState state_;
    int index_ = 0;
};

// 演算法一邊跑一邊用它記錄畫面變化。
// 各種 setter 只改「目前狀態」，呼叫 step() 才把累積的變化存成一步。
// flash 系列只亮一步，下一步會自動恢復原狀。
class Recorder {
public:
    Recorder(const Graph& g, std::string algorithm, std::vector<std::string> pseudocode,
             std::string containerTitle = "");

    void node(int u, NodeState s);
    void flashNode(int u, NodeState s);
    void label(int u, std::string text);
    void edge(int e, EdgeState s);
    void flashEdge(int e, EdgeState s);

    void initMatrix(int n);  // 要在第一個 step() 之前呼叫
    void cell(int i, int j, std::string text);
    void flashMark(int i, int j, CellMark m);

    void initArray(std::vector<int> values);  // 要在第一個 step() 之前呼叫
    void value(int i, int v);
    void bar(int i, BarState s);
    void flashBar(int i, BarState s);
    void pointers(std::vector<Pointer> list);

    void container(std::vector<std::string> items);

    void step(int line, std::string text);

    const VisualState& current() const { return cur_; }
    Trace finish(Result result);

private:
    void setNode(int u, NodeState s);
    void setEdge(int e, EdgeState s);
    void setMark(int idx, CellMark m);
    void setBar(int i, BarState s);

    Trace trace_;
    VisualState cur_;
    Step pending_;
    std::vector<std::pair<int, NodeState>> nodeFlash_;  // (編號, 要還原成的狀態)
    std::vector<std::pair<int, EdgeState>> edgeFlash_;
    std::vector<std::pair<int, CellMark>> markFlash_;
    std::vector<std::pair<int, BarState>> barFlash_;
};

}  // namespace gav
