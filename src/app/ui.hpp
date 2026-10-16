#pragma once

// 配色與簡單的立即模式（immediate mode）元件

#include <string>

#include "app/fonts.hpp"
#include "core/trace.hpp"
#include "raylib.h"

namespace gav::ui {

namespace size {
inline constexpr int Small = 14;
inline constexpr int Body = 16;
inline constexpr int Text = 18;
inline constexpr int Title = 24;
inline constexpr int Symbol = 30;  // 運算式樹的 + - ×
}  // namespace size

namespace color {
inline constexpr Color Background{24, 25, 31, 255};
inline constexpr Color Canvas{30, 32, 40, 255};
inline constexpr Color Panel{36, 38, 48, 255};
inline constexpr Color PanelLight{46, 49, 61, 255};
inline constexpr Color Border{62, 66, 82, 255};
inline constexpr Color Text{230, 232, 238, 255};
inline constexpr Color Dim{150, 156, 170, 255};
inline constexpr Color Accent{98, 160, 255, 255};
inline constexpr Color Highlight{242, 193, 78, 255};

// 點的狀態
inline constexpr Color NodeIdle{58, 61, 74, 255};
inline constexpr Color NodeFrontier{46, 158, 106, 255};
inline constexpr Color NodeCurrent{242, 193, 78, 255};
inline constexpr Color NodeVisited{63, 111, 181, 255};
inline constexpr Color NodePath{242, 140, 40, 255};

// 邊的狀態
inline constexpr Color EdgeIdle{86, 91, 107, 255};
inline constexpr Color EdgeCandidate{126, 200, 160, 255};
inline constexpr Color EdgeExamining{242, 193, 78, 255};
inline constexpr Color EdgeTree{111, 168, 255, 255};
inline constexpr Color EdgeRejected{184, 74, 74, 255};
inline constexpr Color EdgePath{242, 140, 40, 255};

// 格子地圖
inline constexpr Color CellEmpty{42, 45, 55, 255};
inline constexpr Color CellWall{12, 13, 17, 255};
inline constexpr Color CellMud{98, 76, 48, 255};
inline constexpr Color Start{60, 207, 110, 255};
inline constexpr Color Goal{229, 83, 75, 255};

// 排序長條
inline constexpr Color BarNormal{84, 104, 140, 255};
inline constexpr Color BarActive{122, 152, 204, 255};
inline constexpr Color BarCompare{242, 193, 78, 255};
inline constexpr Color BarSwap{229, 83, 75, 255};
inline constexpr Color BarPivot{172, 112, 232, 255};
inline constexpr Color BarSorted{46, 158, 106, 255};
}  // namespace color

Color nodeColor(NodeState s);
Color edgeColor(EdgeState s);
Color barColor(BarState s);

void init(FontBank* fonts);
FontBank& fonts();

void beginFrame();  // 重設游標狀態
void endFrame();

void text(const std::string& s, float x, float y, int size, Color c);
void textCentered(const std::string& s, Vector2 center, int size, Color c);
float textWidth(const std::string& s, int size);

// 回傳這一幀是否被按下
bool button(Rectangle r, const std::string& label, bool active = false, bool enabled = true);
float buttonWidth(const std::string& label);

}  // namespace gav::ui
