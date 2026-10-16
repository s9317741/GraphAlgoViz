// 排序：泡沫、選擇、插入、合併、快速、堆積

#include <algorithm>
#include <format>
#include <functional>

#include "core/algorithms.hpp"

namespace gav {

namespace {

// 包一層 Recorder，順便統計比較與交換次數
class SortRecorder {
public:
    SortRecorder(std::string name, std::vector<std::string> pseudocode, std::vector<int> input)
        : rec(Graph{}, std::move(name), std::move(pseudocode), "統計"), a(std::move(input)) {
        rec.initArray(a);
    }

    void compare(int i, int j) {
        ++compares;
        rec.flashBar(i, BarState::Compare);
        rec.flashBar(j, BarState::Compare);
    }
    // 跟基準／目前最小值比較：只亮被比的那根，基準保持原本的顏色比較好認
    void compareWith(int i) {
        ++compares;
        rec.flashBar(i, BarState::Compare);
    }
    void swap(int i, int j) {
        std::swap(a[i], a[j]);
        ++moves;
        rec.value(i, a[i]);
        rec.value(j, a[j]);
        rec.flashBar(i, BarState::Swap);
        rec.flashBar(j, BarState::Swap);
    }
    void write(int i, int v) {
        a[i] = v;
        ++moves;
        rec.value(i, v);
        rec.flashBar(i, BarState::Swap);
    }
    // [lo, hi] 標成目前範圍；已經定位的不動
    void range(int lo, int hi) {
        for (int k = 0; k < n(); ++k) {
            if (rec.current().bars[k] == BarState::Sorted) continue;
            rec.bar(k, k >= lo && k <= hi ? BarState::Active : BarState::Normal);
        }
    }
    void step(int line, std::string text) {
        std::vector<std::string> items{std::format("比較：{} 次", compares), std::format("交換／寫入：{} 次", moves)};
        items.insert(items.end(), extra.begin(), extra.end());
        rec.container(std::move(items));
        rec.step(line, std::move(text));
    }
    Trace finish(int line) {
        for (int k = 0; k < n(); ++k) rec.bar(k, BarState::Sorted);
        rec.pointers({});
        extra.clear();
        step(line, std::format("排序完成！共比較 {} 次、交換／寫入 {} 次。", compares, moves));
        Result r;
        r.array = a;
        return rec.finish(std::move(r));
    }

    int n() const { return static_cast<int>(a.size()); }
    std::string at(int i) const { return std::format("a[{}]={}", i, a[i]); }

    Recorder rec;
    std::vector<int> a;
    std::vector<std::string> extra;  // 統計下面額外列出的資訊
    int compares = 0;
    int moves = 0;
};

std::string list(const std::vector<int>& v, std::size_t from) {
    std::string s;
    for (std::size_t i = from; i < v.size(); ++i) s += (s.empty() ? "" : " ") + std::to_string(v[i]);
    return s.empty() ? "（用完了）" : s;
}

}  // namespace

// ---------------------------------------------------------------------------
Trace bubbleSort(std::vector<int> input) {
    SortRecorder s("泡沫排序（Bubble Sort）",
                   {
                       "for i = 0 到 n-2:",
                       "    for j = 0 到 n-2-i:",
                       "        if a[j] > a[j+1]:",
                       "            交換 a[j] 和 a[j+1]",
                       "    這一輪都沒交換 → 已經排好，提早結束",
                   },
                   std::move(input));
    const int n = s.n();
    s.step(0, "從左到右兩兩比較，大的往右換。每一輪結束，這一輪最大的會「浮」到最右邊。");
    for (int i = 0; i < n - 1; ++i) {
        bool swapped = false;
        for (int j = 0; j < n - 1 - i; ++j) {
            s.compare(j, j + 1);
            s.rec.pointers({{j, "j"}, {j + 1, "j+1"}});
            if (s.a[j] > s.a[j + 1]) {
                s.step(2, std::format("{} > {} → 順序反了，要交換。", s.at(j), s.at(j + 1)));
                s.swap(j, j + 1);
                swapped = true;
                s.step(3, std::format("交換，大的 {} 往右移一格。", s.a[j + 1]));
            } else {
                s.step(2, std::format("{} <= {}，順序沒問題，不用換。", s.at(j), s.at(j + 1)));
            }
        }
        s.rec.pointers({});
        s.rec.bar(n - 1 - i, BarState::Sorted);
        if (!swapped) {
            s.step(4, std::format("第 {} 輪完全沒交換 → 全部都排好了，提早結束。", i + 1));
            return s.finish(4);
        }
        s.step(0, std::format("第 {} 輪結束：{} 是剩下的裡面最大的，固定在這裡。", i + 1, s.at(n - 1 - i)));
    }
    return s.finish(0);
}

// ---------------------------------------------------------------------------
Trace selectionSort(std::vector<int> input) {
    SortRecorder s("選擇排序（Selection Sort）",
                   {
                       "for i = 0 到 n-2:",
                       "    min = i",
                       "    for j = i+1 到 n-1:",
                       "        if a[j] < a[min]: min = j",
                       "    交換 a[i] 和 a[min]，a[i] 就定位",
                   },
                   std::move(input));
    const int n = s.n();
    s.step(0, "每一輪從還沒排的部分挑出最小的，放到最前面。");
    for (int i = 0; i < n - 1; ++i) {
        int min = i;
        s.rec.bar(min, BarState::Pivot);
        s.rec.pointers({{i, "i"}, {min, "min"}});
        s.step(1, std::format("第 {} 輪：找 a[{}..{}] 裡最小的。先假設 {} 最小。", i + 1, i, n - 1, s.at(i)));
        for (int j = i + 1; j < n; ++j) {
            s.compareWith(j);
            s.rec.pointers({{i, "i"}, {min, "min"}, {j, "j"}});
            if (s.a[j] < s.a[min]) {
                const int old = min;
                s.rec.bar(min, BarState::Normal);
                min = j;
                s.rec.bar(min, BarState::Pivot);
                s.step(3, std::format("{} 比目前最小的 {} 還小 → min 改成 {}。", s.at(j), s.a[old], j));
            } else {
                s.step(3, std::format("{} 沒有比目前最小的 {} 小。", s.at(j), s.a[min]));
            }
        }
        s.rec.bar(min, BarState::Normal);
        s.rec.pointers({{i, "i"}, {min, "min"}});
        std::string text;
        if (min != i) {
            s.swap(i, min);
            text = std::format("最小的是 {}，跟 a[{}] 交換。", s.a[i], i);
        } else {
            text = std::format("最小的就是 a[{}] 自己，不用換。", i);
        }
        s.rec.bar(i, BarState::Sorted);
        s.step(4, text + std::format("a[{}] 就定位。", i));
    }
    return s.finish(0);
}

// ---------------------------------------------------------------------------
Trace insertionSort(std::vector<int> input) {
    SortRecorder s("插入排序（Insertion Sort）",
                   {
                       "for i = 1 到 n-1:",
                       "    j = i   ← a[0..i-1] 已經排好，要把 a[i] 插進去",
                       "    while j > 0 且 a[j-1] > a[j]:",
                       "        交換 a[j-1] 和 a[j]，j = j - 1",
                       "    a[0..i] 都排好了",
                   },
                   std::move(input));
    const int n = s.n();
    if (n > 0) s.rec.bar(0, BarState::Active);
    s.step(0, "像整理撲克牌：左邊是已經排好的部分（一開始只有 a[0]），每次把右邊下一張插進左邊的正確位置。");
    for (int i = 1; i < n; ++i) {
        int j = i;
        const int v = s.a[i];
        s.rec.pointers({{j, "j"}});
        s.step(1, std::format("拿起 a[{}]={}，要插進左邊排好的部分。", i, v));
        while (j > 0) {
            s.compare(j - 1, j);
            s.rec.pointers({{j, "j"}});
            if (s.a[j - 1] > s.a[j]) {
                s.step(2, std::format("{} > {} → {} 還要往左。", s.at(j - 1), v, v));
                s.swap(j - 1, j);
                --j;
                s.rec.pointers({{j, "j"}});
                s.step(3, std::format("交換，{} 往左移一格。", v));
            } else {
                s.step(2, std::format("{} <= {} → 位置找到了。", s.at(j - 1), v));
                break;
            }
        }
        for (int k = 0; k <= i; ++k) s.rec.bar(k, BarState::Active);
        s.rec.pointers({});
        s.step(4, std::format("{} 插在位置 {}，a[0..{}] 都排好了。", v, j, i));
    }
    return s.finish(4);
}

// ---------------------------------------------------------------------------
Trace mergeSort(std::vector<int> input) {
    SortRecorder s("合併排序（Merge Sort）",
                   {
                       "mergeSort(lo, hi):",
                       "    if lo == hi: return（只有一個，本來就排好）",
                       "    mid = (lo + hi) / 2",
                       "    mergeSort(lo, mid)；mergeSort(mid+1, hi)",
                       "    合併：兩邊最前面的比一比，小的先放",
                       "    其中一邊用完 → 另一邊剩下的直接接上",
                   },
                   std::move(input));

    std::function<void(int, int)> sort = [&](int lo, int hi) {
        s.range(lo, hi);
        if (lo == hi) {
            s.step(1, std::format("[{}] 只有一個元素，本來就排好了。", lo));
            return;
        }
        const int mid = (lo + hi) / 2;
        s.step(2, std::format("把 [{}, {}] 切成 [{}, {}] 和 [{}, {}]，各自排好再合併。", lo, hi, lo, mid, mid + 1, hi));
        sort(lo, mid);
        sort(mid + 1, hi);

        s.range(lo, hi);
        const std::vector<int> L(s.a.begin() + lo, s.a.begin() + mid + 1);
        const std::vector<int> R(s.a.begin() + mid + 1, s.a.begin() + hi + 1);
        std::size_t i = 0, j = 0;
        auto halves = [&] { s.extra = {"左半剩下：" + list(L, i), "右半剩下：" + list(R, j)}; };
        halves();
        s.step(3, std::format("[{}, {}] 和 [{}, {}] 都排好了，開始合併。", lo, mid, mid + 1, hi));
        for (int k = lo; k <= hi; ++k) {
            s.rec.pointers({{k, "k"}});
            std::string text;
            int line = 4;
            if (i < L.size() && j < R.size()) {
                ++s.compares;
                if (L[i] <= R[j]) {
                    text = std::format("左邊的 {} <= 右邊的 {} → 放 {} 到位置 {}。（相等時拿左邊，所以合併排序是穩定的）",
                                       L[i], R[j], L[i], k);
                    s.write(k, L[i++]);
                } else {
                    text = std::format("右邊的 {} < 左邊的 {} → 放 {} 到位置 {}。", R[j], L[i], R[j], k);
                    s.write(k, R[j++]);
                }
            } else if (i < L.size()) {
                line = 5;
                text = std::format("右半用完了，左邊剩下的 {} 直接接上。", L[i]);
                s.write(k, L[i++]);
            } else {
                line = 5;
                text = std::format("左半用完了，右邊剩下的 {} 直接接上。", R[j]);
                s.write(k, R[j++]);
            }
            halves();
            s.step(line, text);
        }
        s.extra.clear();
        s.rec.pointers({});
    };

    s.step(0, "分治法：切成兩半，各自排好，再把兩個排好的序列合併。");
    if (s.n() > 0) sort(0, s.n() - 1);
    return s.finish(0);
}

// ---------------------------------------------------------------------------
Trace quickSort(std::vector<int> input) {
    SortRecorder s("快速排序（Quick Sort）",
                   {
                       "quickSort(lo, hi):",
                       "    if lo >= hi: return",
                       "    pivot = a[hi]，i = lo   ← i 左邊都 < pivot",
                       "    for j = lo 到 hi-1:",
                       "        if a[j] < pivot: 交換 a[i] 和 a[j]，i = i + 1",
                       "    交換 a[i] 和 a[hi]   ← pivot 就定位",
                       "    quickSort(lo, i-1)；quickSort(i+1, hi)",
                   },
                   std::move(input));

    std::function<void(int, int)> sort = [&](int lo, int hi) {
        if (lo > hi) return;
        s.range(lo, hi);
        s.extra = {std::format("目前範圍：[{}, {}]", lo, hi)};
        if (lo == hi) {
            s.rec.bar(lo, BarState::Sorted);
            s.step(1, std::format("[{}] 只剩一個元素，它已經在最終位置。", lo));
            return;
        }
        const int pivot = s.a[hi];
        int i = lo;
        s.rec.bar(hi, BarState::Pivot);
        s.rec.pointers({{i, "i"}, {hi, "pivot"}});
        s.step(2, std::format("選最右邊的 {} 當基準（pivot）。目標：比 {} 小的都移到左邊。", s.at(hi), pivot));
        for (int j = lo; j < hi; ++j) {
            s.compareWith(j);
            s.rec.pointers({{i, "i"}, {j, "j"}, {hi, "pivot"}});
            if (s.a[j] < pivot) {
                const int v = s.a[j];
                if (i != j) s.swap(i, j);
                ++i;
                s.rec.pointers({{i, "i"}, {j, "j"}, {hi, "pivot"}});
                s.step(4, i - 1 != j ? std::format("{} < {} → 換到左邊區的尾端（位置 {}），i 往右移。", v, pivot, i - 1)
                                     : std::format("{} < {}，本來就在左邊區，i 往右移。", v, pivot));
            } else {
                s.step(4, std::format("{} >= {}，留在右邊。", s.at(j), pivot));
            }
        }
        if (i != hi) s.swap(i, hi);
        s.rec.bar(hi, BarState::Active);
        s.rec.bar(i, BarState::Sorted);
        s.rec.pointers({{i, "pivot"}});
        s.step(5, std::format("把基準 {} 換到位置 {}：左邊都比它小、右邊都 >= 它 → 它就定位了。", pivot, i));
        s.rec.pointers({});
        sort(lo, i - 1);
        sort(i + 1, hi);
    };

    s.step(0, "選一個基準，把比它小的丟左邊、大的丟右邊，基準就定位了；再對兩邊遞迴。"
              "這裡固定選最右邊當基準，所以資料本來就排好時會退化成 O(n²)。");
    sort(0, s.n() - 1);
    s.extra.clear();
    return s.finish(0);
}

// ---------------------------------------------------------------------------
Trace heapSort(std::vector<int> input) {
    SortRecorder s("堆積排序（Heap Sort）",
                   {
                       "把陣列看成完全二元樹：a[i] 的子節點是 a[2i+1]、a[2i+2]",
                       "建堆：從最後一個有子節點的位置往前，逐一 siftDown",
                       "for end = n-1 到 1:",
                       "    交換 a[0]（最大）和 a[end]，a[end] 就定位",
                       "    siftDown(0)，堆的範圍縮小成 a[0..end-1]",
                       "siftDown(i)：看 i 和它的兩個子節點誰最大",
                       "    最大的不是 i → 交換，繼續往下沉",
                   },
                   std::move(input));
    const int n = s.n();

    auto siftDown = [&](int i, int size) {
        while (true) {
            const int l = 2 * i + 1, r = 2 * i + 2;
            int largest = i;
            std::vector<Pointer> ptrs{{i, "i"}};
            std::string text = s.at(i);
            if (l < size) {
                s.compare(l, i);
                ptrs.emplace_back(l, "左");
                text += "，左子 " + s.at(l);
                if (s.a[l] > s.a[largest]) largest = l;
            }
            if (r < size) {
                s.compare(r, largest);
                ptrs.emplace_back(r, "右");
                text += "，右子 " + s.at(r);
                if (s.a[r] > s.a[largest]) largest = r;
            }
            s.rec.pointers(ptrs);
            if (largest == i) {
                s.step(5, l < size ? text + "：自己最大，停下來。" : text + " 沒有子節點，停下來。");
                break;
            }
            s.step(5, text + std::format("：最大的是 {}。", s.at(largest)));
            s.swap(i, largest);
            s.rec.pointers({{largest, "i"}});
            s.step(6, std::format("交換，{} 往下沉到位置 {}。", s.a[largest], largest));
            i = largest;
        }
        s.rec.pointers({});
    };

    s.range(0, n - 1);
    s.step(0, "先把陣列整理成「最大堆」：每個父節點都比子節點大，所以最大的一定在 a[0]。"
              "然後一直把 a[0] 換到最後面、縮小堆、再整理。");
    for (int i = n / 2 - 1; i >= 0; --i) {
        s.extra = {"階段：建堆"};
        s.step(1, std::format("建堆：對位置 {} 做 siftDown。", i));
        siftDown(i, n);
    }
    for (int end = n - 1; end >= 1; --end) {
        s.extra = {std::format("階段：取出最大值，堆的範圍 a[0..{}]", end)};
        s.swap(0, end);
        s.rec.bar(end, BarState::Sorted);
        s.step(3, std::format("堆頂是目前最大的 {}，換到 a[{}]，固定下來。", s.a[end], end));
        s.range(0, end - 1);
        siftDown(0, end);
    }
    s.extra.clear();
    return s.finish(2);
}

}  // namespace gav
