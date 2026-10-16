#pragma once

// 把 VisualState 畫出來。這裡只負責畫，不改任何狀態。

#include "core/scenes.hpp"
#include "core/trace.hpp"
#include "raylib.h"

namespace gav {

// 圖的邏輯座標 → 螢幕座標
struct GraphView {
    Rectangle area{};
    float scale = 1;
    Vector2 offset{};
    float radius = 22;

    Vector2 toScreen(Vec2 p) const { return {offset.x + p.x * scale, offset.y + p.y * scale}; }
    int nodeAt(const Graph& g, Vector2 mouse) const;  // 沒點到回傳 -1
};
GraphView fitGraph(const Graph& g, Rectangle area);

struct GridView {
    Rectangle area{};
    float cell = 20;
    Vector2 origin{};

    int cellAt(const GridMap& m, Vector2 mouse) const;  // 在地圖外回傳 -1
    int cellAtClamped(const GridMap& m, Vector2 mouse) const;
};
GridView fitGrid(const GridMap& m, Rectangle area);

// 要標示哪些點是起點／終點（-1 = 不標）
struct Markers {
    int start = -1;
    int goal = -1;
};

void drawGraph(const Scene& s, const VisualState& v, const GraphView& view, Markers m);
void drawGrid(const Scene& s, const VisualState& v, const GridView& view, Markers m);
void drawMatrix(const Graph& g, const VisualState& v, Rectangle area);
void drawArray(const VisualState& v, Rectangle area);
void drawPanel(const Trace& t, const VisualState& v, Algo algo, int stepIndex, int stepCount, Rectangle area);

enum class LegendKind { Graph, Grid, Tree, Array };
void drawLegend(Rectangle canvas, LegendKind kind);

}  // namespace gav
