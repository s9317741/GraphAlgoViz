#include "app/render.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <format>
#include <string>
#include <vector>

#include "app/ui.hpp"

namespace gav {

namespace {

using namespace ui;

Vector2 add(Vector2 a, Vector2 b) { return {a.x + b.x, a.y + b.y}; }
Vector2 sub(Vector2 a, Vector2 b) { return {a.x - b.x, a.y - b.y}; }
Vector2 mul(Vector2 a, float k) { return {a.x * k, a.y * k}; }
float length(Vector2 a) { return std::sqrt(a.x * a.x + a.y * a.y); }

Color mix(Color a, Color b, float t) {
    auto lerp = [t](unsigned char x, unsigned char y) { return static_cast<unsigned char>(x + (y - x) * t); };
    return {lerp(a.r, b.r), lerp(a.g, b.g), lerp(a.b, b.b), 255};
}

// 亮色底配深色字，暗色底配淺色字
Color textOn(Color bg) {
    const float lum = 0.299f * bg.r + 0.587f * bg.g + 0.114f * bg.b;
    return lum > 150 ? Color{20, 22, 28, 255} : color::Text;
}

// raylib 的三角形要逆時針才畫得出來；兩種順序都畫，被剔除的那個不會出現
void triangle(Vector2 a, Vector2 b, Vector2 c, Color col) {
    DrawTriangle(a, b, c, col);
    DrawTriangle(a, c, b, col);
}

void dashedLine(Vector2 a, Vector2 b, float thick, Color col) {
    const Vector2 d = sub(b, a);
    const float len = length(d);
    if (len < 1) return;
    const Vector2 dir = mul(d, 1 / len);
    constexpr float Dash = 9, Gap = 6;
    for (float t = 0; t < len; t += Dash + Gap)
        DrawLineEx(add(a, mul(dir, t)), add(a, mul(dir, std::min(t + Dash, len))), thick, col);
}

float edgeThickness(EdgeState s) {
    switch (s) {
        case EdgeState::Examining: return 5;
        case EdgeState::Tree: return 4;
        case EdgeState::Path: return 6;
        default: return 2.5f;
    }
}

void pill(const std::string& s, Vector2 center, int size, Color bg, Color fg) {
    const float w = textWidth(s, size) + 10, h = float(size) + 6;
    DrawRectangleRounded({center.x - w / 2, center.y - h / 2, w, h}, 0.5f, 6, bg);
    textCentered(s, center, size, fg);
}

}  // namespace

// ---------------------------------------------------------------------------
// 一般圖
// ---------------------------------------------------------------------------

GraphView fitGraph(const Graph& g, Rectangle area) {
    GraphView view;
    view.area = area;
    if (g.nodes.empty()) return view;
    float minX = g.nodes[0].pos.x, maxX = minX, minY = g.nodes[0].pos.y, maxY = minY;
    for (const Node& n : g.nodes) {
        minX = std::min(minX, n.pos.x);
        maxX = std::max(maxX, n.pos.x);
        minY = std::min(minY, n.pos.y);
        maxY = std::max(maxY, n.pos.y);
    }
    // 上面留給場景說明，下面留給圖例
    const float left = 60, right = 60, top = 80, bottom = 80;
    const float w = std::max(1.f, area.width - left - right), h = std::max(1.f, area.height - top - bottom);
    view.scale = std::min({w / std::max(1.f, maxX - minX), h / std::max(1.f, maxY - minY), 1.6f});
    view.offset = {area.x + left + (w - (maxX - minX) * view.scale) / 2 - minX * view.scale,
                   area.y + top + (h - (maxY - minY) * view.scale) / 2 - minY * view.scale};
    view.radius = std::clamp(26 * view.scale, 16.f, 24.f);
    return view;
}

int GraphView::nodeAt(const Graph& g, Vector2 mouse) const {
    for (int u = 0; u < g.size(); ++u)
        if (CheckCollisionPointCircle(mouse, toScreen(g.nodes[u].pos), radius + 4)) return u;
    return -1;
}

void drawGraph(const Scene& s, const VisualState& v, const GraphView& view, Markers m) {
    const Graph& g = s.graph;
    const float R = view.radius;

    // 有向圖若 u→v 和 v→u 都有，兩條線各往旁邊偏一點才不會疊在一起
    auto hasReverse = [&](const Edge& e) {
        if (!g.directed) return false;
        return std::any_of(g.edges.begin(), g.edges.end(), [&](const Edge& f) { return f.from == e.to && f.to == e.from; });
    };

    // 先畫一般的邊，再畫有狀態的邊，重要的才會在上層
    std::vector<int> order(g.edges.size());
    for (std::size_t i = 0; i < order.size(); ++i) order[i] = static_cast<int>(i);
    std::stable_sort(order.begin(), order.end(), [&](int a, int b) {
        auto rank = [](EdgeState st) { return st == EdgeState::Normal ? 0 : st == EdgeState::Rejected ? 1 : 2; };
        return rank(v.edges[a]) < rank(v.edges[b]);
    });

    for (int e : order) {
        const Edge& ed = g.edges[e];
        const EdgeState st = v.edges[e];
        Vector2 a = view.toScreen(g.nodes[ed.from].pos), b = view.toScreen(g.nodes[ed.to].pos);
        const Vector2 d = sub(b, a);
        const float len = length(d);
        if (len < 1) continue;
        const Vector2 dir = mul(d, 1 / len), perp = {-dir.y, dir.x};
        const bool offset = hasReverse(ed);
        if (offset) {
            a = add(a, mul(perp, 7));
            b = add(b, mul(perp, 7));
        }
        const Color col = edgeColor(st);
        const float thick = edgeThickness(st);
        const Vector2 from = add(a, mul(dir, R));
        Vector2 to = sub(b, mul(dir, R + (g.directed ? 2 : 0)));
        if (g.directed) {
            const float head = 10 + thick;
            const Vector2 base = sub(to, mul(dir, head));
            triangle(to, add(base, mul(perp, head * 0.5f)), sub(base, mul(perp, head * 0.5f)), col);
            to = add(base, mul(dir, 2));
        }
        if (st == EdgeState::Candidate) dashedLine(from, to, thick, col);
        else DrawLineEx(from, to, thick, col);

        if (s.showWeights) {
            const Vector2 mid = add(mul(add(a, b), 0.5f), mul(perp, offset ? 12 : 0));
            const Color fg = ed.weight < 0 ? color::Goal : st == EdgeState::Normal ? color::Dim : col;
            pill(std::format("{}", ed.weight), mid, size::Small, color::Canvas, fg);
        }
    }

    for (int u = 0; u < g.size(); ++u) {
        const Vector2 c = view.toScreen(g.nodes[u].pos);
        const Color fill = nodeColor(v.nodes[u]);
        if (u == m.start) DrawRing(c, R + 3, R + 7, 0, 360, 48, color::Start);
        if (u == m.goal) DrawRing(c, R + 3, R + 7, 0, 360, 48, color::Goal);
        DrawCircleV(c, R, fill);
        DrawRing(c, R - 2, R, 0, 360, 48, mix(fill, WHITE, 0.25f));
        // 運算子（× + -）字形本身很小，單一符號就用大字
        const std::string& name = g.name(u);
        const bool symbol = name == "×" || (name.size() == 1 && !std::isalnum(static_cast<unsigned char>(name[0])));
        const int nameSize = symbol ? size::Symbol : textWidth(name, size::Text) > 2 * R - 6 ? size::Small : size::Text;
        textCentered(g.name(u), c, nameSize, textOn(fill));

        if (!v.nodeLabels[u].empty()) pill(v.nodeLabels[u], {c.x, c.y + R + 14}, size::Small, color::PanelLight, color::Text);
        if (u == m.start) textCentered("起點", {c.x, c.y - R - 18}, size::Small, color::Start);
        if (u == m.goal) textCentered("終點", {c.x, c.y - R - 18}, size::Small, color::Goal);
    }
}

// ---------------------------------------------------------------------------
// 格子地圖
// ---------------------------------------------------------------------------

GridView fitGrid(const GridMap& m, Rectangle area) {
    GridView view;
    view.area = area;
    const float top = 46, bottom = 46, side = 16;
    view.cell = std::floor(std::min((area.width - 2 * side) / m.cols, (area.height - top - bottom) / m.rows));
    view.cell = std::max(view.cell, 4.f);
    view.origin = {area.x + (area.width - view.cell * m.cols) / 2, area.y + top + (area.height - top - bottom - view.cell * m.rows) / 2};
    return view;
}

int GridView::cellAt(const GridMap& m, Vector2 mouse) const {
    const int c = static_cast<int>(std::floor((mouse.x - origin.x) / cell));
    const int r = static_cast<int>(std::floor((mouse.y - origin.y) / cell));
    if (r < 0 || r >= m.rows || c < 0 || c >= m.cols) return -1;
    return m.id(r, c);
}

int GridView::cellAtClamped(const GridMap& m, Vector2 mouse) const {
    const int c = std::clamp(static_cast<int>(std::floor((mouse.x - origin.x) / cell)), 0, m.cols - 1);
    const int r = std::clamp(static_cast<int>(std::floor((mouse.y - origin.y) / cell)), 0, m.rows - 1);
    return m.id(r, c);
}

void drawGrid(const Scene& s, const VisualState& v, const GridView& view, Markers mk) {
    const GridMap& m = *s.grid;
    const float gap = view.cell >= 8 ? 1 : 0;
    for (int r = 0; r < m.rows; ++r) {
        for (int c = 0; c < m.cols; ++c) {
            const int id = m.id(r, c);
            const Rectangle rect{view.origin.x + c * view.cell, view.origin.y + r * view.cell, view.cell - gap, view.cell - gap};
            const GridMap::Cell cell = m.at(id);
            Color col = color::CellEmpty;
            if (cell == GridMap::Cell::Wall) {
                col = color::CellWall;
            } else {
                const bool mud = cell == GridMap::Cell::Mud;
                if (v.nodes[id] != NodeState::Unvisited) col = mud ? mix(nodeColor(v.nodes[id]), color::CellMud, 0.45f) : nodeColor(v.nodes[id]);
                else if (mud) col = color::CellMud;
            }
            if (id == mk.start) col = color::Start;
            if (id == mk.goal) col = color::Goal;
            DrawRectangleRec(rect, col);

            const Vector2 center{rect.x + rect.width / 2, rect.y + rect.height / 2};
            if (view.cell >= 16 && (id == mk.start || id == mk.goal))
                textCentered(id == mk.start ? "起" : "終", center, size::Small, Color{16, 18, 24, 255});
            else if (view.cell >= 36 && !v.nodeLabels[id].empty())
                textCentered(v.nodeLabels[id], center, size::Small, textOn(col));
        }
    }
}

// ---------------------------------------------------------------------------
// Floyd-Warshall 的距離矩陣
// ---------------------------------------------------------------------------

void drawMatrix(const Graph& g, const VisualState& v, Rectangle area) {
    const int n = v.matrixSize;
    if (n == 0) return;
    const float cs = std::floor(std::min({64.f, (area.width - 32) / (n + 1), (area.height - 140) / (n + 1)}));
    const float x0 = area.x + (area.width - cs * (n + 1)) / 2, y0 = area.y + 80;
    text("距離矩陣 d[列][行]：第 i 列第 j 行 = i 到 j 的最短距離", x0, y0 - 30, size::Body, color::Text);
    const int fs = cs >= 44 ? size::Body : size::Small;

    auto header = [&](int u, float x, float y) {
        const bool k = v.nodes[u] == NodeState::Current;
        DrawRectangleRec({x, y, cs - 2, cs - 2}, k ? color::Highlight : color::PanelLight);
        textCentered(g.name(u), {x + cs / 2 - 1, y + cs / 2 - 1}, fs, k ? Color{20, 22, 28, 255} : color::Text);
    };
    for (int i = 0; i < n; ++i) {
        header(i, x0 + (i + 1) * cs, y0);
        header(i, x0, y0 + (i + 1) * cs);
    }
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            const int idx = i * n + j;
            const Rectangle r{x0 + (j + 1) * cs, y0 + (i + 1) * cs, cs - 2, cs - 2};
            Color bg = color::CellEmpty;
            switch (v.marks[idx]) {
                case CellMark::Via: bg = Color{44, 90, 160, 255}; break;
                case CellMark::Target: bg = Color{138, 109, 31, 255}; break;
                case CellMark::Updated: bg = Color{47, 143, 104, 255}; break;
                case CellMark::None: break;
            }
            DrawRectangleRec(r, bg);
            const std::string& val = v.matrix[idx];
            Color fg = val == "∞" ? color::Dim : color::Text;
            if (i == j && !val.empty() && val[0] == '-') fg = color::Goal;
            textCentered(val, {r.x + r.width / 2, r.y + r.height / 2}, fs, fg);
        }
    }
    const float ly = y0 + (n + 1) * cs + 14;
    struct Key {
        Color c;
        const char* label;
    };
    float lx = x0;
    for (const Key& k : {Key{{44, 90, 160, 255}, "拿來組合的兩段 d[i][k]、d[k][j]"}, Key{{138, 109, 31, 255}, "比較中 d[i][j]"},
                         Key{{47, 143, 104, 255}, "剛更新"}}) {
        DrawRectangleRec({lx, ly + 2, 14, 14}, k.c);
        text(k.label, lx + 20, ly, size::Small, color::Dim);
        lx += 20 + textWidth(k.label, size::Small) + 18;
    }
    text("黃色表頭 = 目前的中繼點 k", x0, ly + 24, size::Small, color::Dim);
}

// ---------------------------------------------------------------------------
// 排序的長條圖
// ---------------------------------------------------------------------------

void drawArray(const VisualState& v, Rectangle area) {
    const int n = static_cast<int>(v.array.size());
    if (n == 0) return;
    const int maxV = std::max(1, *std::max_element(v.array.begin(), v.array.end()));
    const float side = 40, top = 70, bottom = 120;
    const float slot = (area.width - 2 * side) / float(n);
    const float barW = slot * (slot > 14 ? 0.78f : 0.9f);
    const bool roomy = slot >= 26;  // 夠寬才寫數字和索引
    const float baseY = area.y + area.height - bottom;
    const float maxH = baseY - (area.y + top) - 24;

    for (int i = 0; i < n; ++i) {
        const float h = std::max(3.f, float(v.array[i]) / float(maxV) * maxH);
        const float x = area.x + side + float(i) * slot + (slot - barW) / 2;
        DrawRectangleRounded({x, baseY - h, barW, h}, roomy ? 0.12f : 0, 4, barColor(v.bars[i]));
        if (roomy) {
            textCentered(std::to_string(v.array[i]), {x + barW / 2, baseY - h - 12}, size::Small, color::Text);
            textCentered(std::to_string(i), {x + barW / 2, baseY + 12}, size::Small, color::Dim);
        }
    }

    // 指標（i、j、pivot…）：同一格有好幾個就往下疊
    std::vector<int> stacked(n, 0);
    for (const auto& [i, label] : v.pointers) {
        if (i < 0 || i >= n) continue;
        const float cx = area.x + side + (float(i) + 0.5f) * slot;
        const float cy = baseY + (roomy ? 40 : 16) + float(stacked[i]++) * 24;
        triangle({cx, cy - 16}, {cx - 5, cy - 9}, {cx + 5, cy - 9}, color::Highlight);
        pill(label, {cx, cy}, size::Small, color::Highlight, Color{20, 22, 28, 255});
    }
}

// ---------------------------------------------------------------------------
// 右側說明面板
// ---------------------------------------------------------------------------

void drawPanel(const Trace& t, const VisualState& v, Algo algo, int stepIndex, int stepCount, Rectangle area) {
    DrawRectangleRec(area, color::Panel);
    DrawLineEx({area.x, area.y}, {area.x, area.y + area.height}, 1, color::Border);
    BeginScissorMode(int(area.x), int(area.y), int(area.width), int(area.height));

    const float x = area.x + 18, w = area.width - 36;
    float y = area.y + 14;
    const AlgoInfo& info = algoInfo(algo);

    text(t.algorithm, x, y, size::Title, color::Text);
    y += 34;
    text(info.complexity, x, y, size::Small, color::Accent);
    y += 22;
    for (const auto& line : fonts().wrap(info.idea, size::Body, w)) {
        text(line, x, y, size::Body, color::Dim);
        y += 22;
    }
    y += 10;

    // 這一步在做什麼
    const auto lines = fonts().wrap(v.text, size::Text, w - 24);
    const float boxH = 38 + 26 * float(std::max<std::size_t>(lines.size(), 1));
    DrawRectangleRounded({x, y, w, boxH}, 0.08f, 6, color::PanelLight);
    DrawRectangleRec({x, y + 6, 4, boxH - 12}, color::Highlight);
    text(std::format("步驟 {} / {}", stepIndex, stepCount), x + 14, y + 8, size::Small, color::Highlight);
    float ty = y + 32;
    for (const auto& line : lines) {
        text(line, x + 14, ty, size::Text, color::Text);
        ty += 26;
    }
    y += boxH + 16;

    // 虛擬碼：縮排用固定寬度畫，比字型裡的空白好對齊
    text("虛擬碼", x, y, size::Small, color::Dim);
    y += 22;
    constexpr float Indent = 18;
    for (int i = 0; i < static_cast<int>(t.pseudocode.size()); ++i) {
        const std::string& raw = t.pseudocode[i];
        const std::size_t spaces = raw.find_first_not_of(' ');
        const float indent = Indent * float(spaces == std::string::npos ? 0 : spaces / 4);
        const auto wrapped = fonts().wrap(raw.substr(spaces == std::string::npos ? 0 : spaces), size::Body, w - 16 - indent - 12);
        const float h = 22 * float(wrapped.size());
        const bool current = i == v.line;
        if (current) DrawRectangleRounded({x, y - 2, w, h + 2}, 0.2f, 6, Color{242, 193, 78, 50});
        for (std::size_t k = 0; k < wrapped.size(); ++k)
            text(wrapped[k], x + 8 + indent + (k > 0 ? 12 : 0), y + 22 * float(k), size::Body,
                 current ? color::Highlight : color::Text);
        y += h;
    }
    y += 16;

    if (!t.containerTitle.empty()) {
        text(t.containerTitle, x, y, size::Small, color::Dim);
        y += 22;
        if (v.container.empty()) {
            text("（空）", x + 8, y, size::Body, color::Dim);
        } else {
            for (const auto& item : v.container) {
                for (const auto& line : fonts().wrap(item, size::Body, w - 16)) {
                    text(line, x + 8, y, size::Body, color::Text);
                    y += 21;
                }
            }
        }
    }
    EndScissorMode();
}

// ---------------------------------------------------------------------------
// 圖例
// ---------------------------------------------------------------------------

void drawLegend(Rectangle canvas, LegendKind kind) {
    struct Item {
        Color c;
        const char* label;
        bool line;
    };
    std::vector<Item> items;
    if (kind == LegendKind::Array) {
        items = {{color::BarNormal, "未排序", false},       {color::BarActive, "目前範圍／已排好的部分", false},
                 {color::BarCompare, "比較中", false},      {color::BarSwap, "交換／寫入", false},
                 {color::BarPivot, "基準／目前最小", false}, {color::BarSorted, "已就定位", false}};
    } else if (kind == LegendKind::Tree) {
        items = {{color::NodeIdle, "還沒到", false},       {color::NodeFrontier, "等待中（在堆疊／佇列裡）", false},
                 {color::NodeCurrent, "現在這一個", false}, {color::NodeVisited, "已輸出", false},
                 {color::NodePath, "完成", false},        {color::EdgeTree, "目前走的路", true}};
    } else {
        items = {{color::NodeIdle, "未發現", false},  {color::NodeFrontier, "待處理", false},
                 {color::NodeCurrent, "處理中", false}, {color::NodeVisited, "已完成", false},
                 {color::NodePath, "答案", false}};
    }
    if (kind == LegendKind::Grid) {
        items.insert(items.end(), {{color::CellWall, "牆", false},
                                   {color::CellMud, "泥地（成本 5）", false},
                                   {color::Start, "起點", false},
                                   {color::Goal, "終點", false}});
    } else if (kind == LegendKind::Graph) {
        items.insert(items.end(), {{color::EdgeExamining, "檢查中", true},
                                   {color::EdgeTree, "樹／已選", true},
                                   {color::EdgeCandidate, "候選", true},
                                   {color::EdgeRejected, "捨棄", true}});
    }
    float x = canvas.x + 16;
    const float y = canvas.y + canvas.height - 30;
    for (const Item& it : items) {
        if (it.line) DrawLineEx({x, y + 8}, {x + 18, y + 8}, 4, it.c);
        else DrawRectangleRounded({x, y + 1, 14, 14}, 0.3f, 4, it.c);
        const float lx = x + (it.line ? 24 : 20);
        text(it.label, lx, y, size::Small, color::Dim);
        x = lx + textWidth(it.label, size::Small) + 18;
    }
}

}  // namespace gav
