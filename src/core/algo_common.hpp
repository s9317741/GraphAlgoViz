#pragma once

// 各演算法共用的小工具（只給 core 內部用）

#include <algorithm>
#include <string>
#include <vector>

#include "core/graph.hpp"
#include "core/trace.hpp"

namespace gav {

// 二元堆積（最小堆）。不用 std::priority_queue 是因為畫面要列出佇列內容。
template <typename T, typename Less>
class MinHeap {
public:
    void push(T item) {
        items_.push_back(std::move(item));
        std::push_heap(items_.begin(), items_.end(), Greater{});
    }
    T pop() {
        std::pop_heap(items_.begin(), items_.end(), Greater{});
        T top = std::move(items_.back());
        items_.pop_back();
        return top;
    }
    bool empty() const { return items_.empty(); }
    // 由小到大排好的複本，給畫面顯示用
    std::vector<T> sorted() const {
        std::vector<T> copy = items_;
        std::sort(copy.begin(), copy.end(), Less{});
        return copy;
    }

private:
    struct Greater {
        bool operator()(const T& a, const T& b) const { return Less{}(b, a); }
    };
    std::vector<T> items_;
};

// 距離的顯示格式：∞、整數不帶小數、其他取一位小數
std::string fmtNum(double v);

// "A → C → F"；太長時中間省略
std::string pathString(const Graph& g, const std::vector<int>& path);

// 由 parentEdge（每個點是從哪條邊走來的）回推 src → dst 的路徑，
// 把路徑上的點和邊標成 Path，並填好 res.path 與 res.pathCost。
// 走不到就回傳 false，畫面不動。
bool markPath(Recorder& rec, const Graph& g, const std::vector<int>& parentEdge, int src, int dst,
              Result& res);

// 所有邊權重都是 1（例如沒有泥地的格子地圖）
bool allUnitWeights(const Graph& g);

}  // namespace gav
