#include "app/ui.hpp"

namespace gav::ui {

namespace {
FontBank* g_fonts = nullptr;
bool g_hoverClickable = false;
}  // namespace

Color nodeColor(NodeState s) {
    switch (s) {
        case NodeState::Unvisited: return color::NodeIdle;
        case NodeState::Frontier: return color::NodeFrontier;
        case NodeState::Current: return color::NodeCurrent;
        case NodeState::Visited: return color::NodeVisited;
        case NodeState::Path: return color::NodePath;
    }
    return color::NodeIdle;
}

Color edgeColor(EdgeState s) {
    switch (s) {
        case EdgeState::Normal: return color::EdgeIdle;
        case EdgeState::Candidate: return color::EdgeCandidate;
        case EdgeState::Examining: return color::EdgeExamining;
        case EdgeState::Tree: return color::EdgeTree;
        case EdgeState::Rejected: return color::EdgeRejected;
        case EdgeState::Path: return color::EdgePath;
    }
    return color::EdgeIdle;
}

Color barColor(BarState s) {
    switch (s) {
        case BarState::Normal: return color::BarNormal;
        case BarState::Active: return color::BarActive;
        case BarState::Compare: return color::BarCompare;
        case BarState::Swap: return color::BarSwap;
        case BarState::Pivot: return color::BarPivot;
        case BarState::Sorted: return color::BarSorted;
    }
    return color::BarNormal;
}

void init(FontBank* fonts) { g_fonts = fonts; }
FontBank& fonts() { return *g_fonts; }

void beginFrame() { g_hoverClickable = false; }
void endFrame() { SetMouseCursor(g_hoverClickable ? MOUSE_CURSOR_POINTING_HAND : MOUSE_CURSOR_DEFAULT); }

void text(const std::string& s, float x, float y, int size, Color c) { g_fonts->draw(s, {x, y}, size, c); }

void textCentered(const std::string& s, Vector2 center, int size, Color c) {
    const float w = g_fonts->width(s, size);
    g_fonts->draw(s, {center.x - w / 2, center.y - float(size) / 2 - 1}, size, c);
}

float textWidth(const std::string& s, int size) { return g_fonts->width(s, size); }

float buttonWidth(const std::string& label) { return textWidth(label, size::Body) + 24; }

bool button(Rectangle r, const std::string& label, bool active, bool enabled) {
    const bool hover = enabled && CheckCollisionPointRec(GetMousePosition(), r);
    if (hover) g_hoverClickable = true;
    const Color bg = active ? Color{52, 112, 214, 255} : hover ? color::PanelLight : color::Panel;
    const Color fg = active ? WHITE : enabled ? color::Text : color::Dim;
    DrawRectangleRounded(r, 0.25f, 6, bg);
    if (!active) DrawRectangleRoundedLinesEx(r, 0.25f, 6, 1, color::Border);
    textCentered(label, {r.x + r.width / 2, r.y + r.height / 2}, size::Body, fg);
    return hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

}  // namespace gav::ui
