#include "app/app.hpp"

#include <algorithm>
#include <cmath>
#include <ctime>
#include <format>

#include "app/ui.hpp"

namespace gav {

namespace {

using namespace ui;

constexpr double Speeds[] = {0.5, 1, 2, 3, 5, 8, 15, 30, 60, 120, 240, 480, 960};  // 每秒幾步
constexpr int SpeedCount = static_cast<int>(std::size(Speeds));

constexpr float TopH = 52, AlgoH = 44, SceneH = 46, BottomH = 60, PanelW = 470;

// 只有圖論的單源演算法需要起點；樹和排序都不用
bool usesStart(Algo a) { return a <= Algo::Prim; }
bool usesGoal(Algo a) { return a <= Algo::FloydWarshall; }

int defaultSpeed(Algo a, SceneKind k) {
    if (k == SceneKind::Grid) return 8;          // 60 步/秒：格子地圖步數很多
    if (k == SceneKind::ArrayLarge) return 7;    // 30 步/秒
    if (a == Algo::FloydWarshall || a >= Algo::Bubble) return 4;  // 5 步/秒
    return 2;                                    // 2 步/秒：慢慢看說明
}

std::string speedText(double s) { return s < 1 ? std::format("{:.1f} 步/秒", s) : std::format("{} 步/秒", s); }

}  // namespace

App::App() : seed_(static_cast<unsigned>(std::time(nullptr))) {
    fonts_.load();
    ui::init(&fonts_);
    for (int k = 0; k < SceneCount; ++k) scenes_[k] = makeScene(static_cast<SceneKind>(k));
    prewarmFonts();
    selectAlgo(Algo::Dijkstra);
}

App::~App() { fonts_.unload(); }

// 先把每個演算法的說明文字都跑一遍，讓字型一次烘好，播放時就不會一直重建
void App::prewarmFonts() {
    for (int a = 0; a < AlgoCount; ++a) {
        const AlgoInfo& info = algoInfo(static_cast<Algo>(a));
        fonts_.require(std::string(info.shortName) + info.idea + info.complexity);
        for (SceneKind k : scenesFor(static_cast<Algo>(a))) {
            const Scene& s = scenes_[static_cast<int>(k)];
            fonts_.require(s.name + s.note);
            trace_ = runAlgorithm(static_cast<Algo>(a), s);
            requireTrace();
        }
    }
}

void App::requireTrace() {
    fonts_.require(trace_.algorithm + trace_.containerTitle);
    for (const auto& line : trace_.pseudocode) fonts_.require(line);
    for (const Step& st : trace_.steps) {
        fonts_.require(st.text);
        for (const auto& [u, label] : st.nodeLabels) fonts_.require(label);
        if (st.container)
            for (const auto& item : *st.container) fonts_.require(item);
        if (st.pointers)
            for (const auto& [i, label] : *st.pointers) fonts_.require(label);
    }
}

Markers App::markers() const {
    const Scene& s = scenes_[static_cast<int>(sceneKind_)];
    return {usesStart(algo_) ? s.start : -1, usesGoal(algo_) ? s.goal : -1};
}

void App::selectAlgo(Algo a) {
    algo_ = a;
    const auto kinds = scenesFor(a);
    // 目前的場景也適用就留著，方便同一張圖比較不同演算法
    if (std::find(kinds.begin(), kinds.end(), sceneKind_) == kinds.end()) sceneKind_ = kinds.front();
    setSpeed(defaultSpeed(algo_, sceneKind_));
    rerun(false);
}

void App::selectScene(SceneKind k) {
    sceneKind_ = k;
    setSpeed(defaultSpeed(algo_, sceneKind_));
    rerun(false);
}

void App::rerun(bool showEnd) {
    trace_ = runAlgorithm(algo_, scene());
    player_.load(&trace_);
    acc_ = 0;
    if (showEnd) {
        player_.seek(player_.count());
        playing_ = false;
    } else {
        requireTrace();
        player_.seek(1);
        playing_ = true;
    }
}

void App::togglePlay() {
    if (player_.index() >= player_.count()) player_.seek(0);
    playing_ = !playing_;
    acc_ = 0;
}

void App::stepBy(int delta) {
    playing_ = false;
    player_.seek(player_.index() + delta);
}

void App::setSpeed(int idx) { speedIdx_ = std::clamp(idx, 0, SpeedCount - 1); }

void App::updatePlayback() {
    if (!playing_) return;
    acc_ += std::min(GetFrameTime(), 0.1f) * Speeds[speedIdx_];
    const int n = static_cast<int>(acc_);
    acc_ -= n;
    if (n > 0) player_.seek(player_.index() + n);
    if (player_.index() >= player_.count()) playing_ = false;
}

App::Layout App::layout() const {
    const float W = float(GetScreenWidth()), H = float(GetScreenHeight());
    Layout L{};
    L.top = {0, 0, W, TopH};
    L.algoBar = {0, TopH, W, AlgoH};
    L.sceneBar = {0, TopH + AlgoH, W, SceneH};
    const float bodyY = TopH + AlgoH + SceneH, bodyH = H - bodyY - BottomH;
    L.panel = {W - PanelW, bodyY, PanelW, bodyH};
    L.canvas = {0, bodyY, W - PanelW, bodyH};
    L.graphArea = L.canvas;
    if (trace_.initial.matrixSize > 0) {
        L.graphArea.width = L.canvas.width * 0.5f;
        L.matrixArea = {L.canvas.x + L.graphArea.width, L.canvas.y, L.canvas.width - L.graphArea.width, L.canvas.height};
    }
    L.bottom = {0, H - BottomH, W, BottomH};
    const float progX = 640;
    L.progress = {progX, H - BottomH / 2 - 5, std::max(60.f, W - progX - 170), 10};
    return L;
}

// ---------------------------------------------------------------------------
// 輸入
// ---------------------------------------------------------------------------

void App::handleKeys() {
    if (IsKeyPressed(KEY_SPACE)) togglePlay();
    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressedRepeat(KEY_RIGHT)) stepBy(1);
    if (IsKeyPressed(KEY_LEFT) || IsKeyPressedRepeat(KEY_LEFT)) stepBy(-1);
    if (IsKeyPressed(KEY_HOME)) stepBy(-player_.count());
    if (IsKeyPressed(KEY_END)) stepBy(player_.count());
    if (IsKeyPressed(KEY_UP)) setSpeed(speedIdx_ + 1);
    if (IsKeyPressed(KEY_DOWN)) setSpeed(speedIdx_ - 1);
    if (IsKeyPressed(KEY_R)) {
        player_.seek(0);
        playing_ = true;
    }
    const auto& cats = categories();
    const auto& algos = cats[categoryOf(algo_)].algos;
    for (int i = 0; i < static_cast<int>(algos.size()) && i < 9; ++i)
        if (IsKeyPressed(KEY_ONE + i)) selectAlgo(algos[i]);
    const int cat = categoryOf(algo_), catCount = static_cast<int>(cats.size());
    if (IsKeyPressed(KEY_LEFT_BRACKET)) selectAlgo(cats[(cat + catCount - 1) % catCount].algos.front());
    if (IsKeyPressed(KEY_RIGHT_BRACKET)) selectAlgo(cats[(cat + 1) % catCount].algos.front());
    if (IsKeyPressed(KEY_TAB)) {
        const auto kinds = scenesFor(algo_);
        const auto it = std::find(kinds.begin(), kinds.end(), sceneKind_);
        selectScene(kinds[(it - kinds.begin() + 1) % kinds.size()]);
    }
    if (scene().grid) {
        if (IsKeyPressed(KEY_C)) gridCommand(0);
        if (IsKeyPressed(KEY_N)) gridCommand(1);
        if (IsKeyPressed(KEY_M)) gridCommand(2);
    } else if (scene().reshufflable && IsKeyPressed(KEY_N)) {
        reshuffle();
    }
}

void App::reshuffle() {
    scene() = makeScene(sceneKind_, seed_++);
    rerun(false);
}

void App::handleGraphMouse() {
    if (!usesStart(algo_)) return;
    const Vector2 mouse = GetMousePosition();
    if (!CheckCollisionPointRec(mouse, L_.graphArea)) return;
    const bool left = IsMouseButtonPressed(MOUSE_BUTTON_LEFT), right = IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);
    if (!left && !right) return;
    const int u = graphView_.nodeAt(scene().graph, mouse);
    if (u < 0) return;
    Scene& s = scene();
    if (left && u != s.goal) s.start = u;
    if (right && u != s.start && usesGoal(algo_)) s.goal = u;
    rerun(false);
}

// 滑鼠移動很快時兩幀之間會跳過好幾格，沿直線把中間的格子也補上
void App::paintLine(int from, int to, bool& changed) {
    GridMap& m = *scene().grid;
    const Scene& s = scene();
    int r0 = from / m.cols, c0 = from % m.cols;
    const int r1 = to / m.cols, c1 = to % m.cols;
    const int steps = std::max(std::abs(r1 - r0), std::abs(c1 - c0));
    for (int i = 0; i <= steps; ++i) {
        const int r = steps == 0 ? r0 : r0 + (r1 - r0) * i / steps;
        const int c = steps == 0 ? c0 : c0 + (c1 - c0) * i / steps;
        const int id = m.id(r, c);
        if (id == s.start || id == s.goal || m.at(id) == paintValue_) continue;
        m.at(id) = paintValue_;
        changed = true;
    }
}

void App::handleGridMouse() {
    Scene& s = scene();
    GridMap& m = *s.grid;
    const Vector2 mouse = GetMousePosition();
    const bool inCanvas = CheckCollisionPointRec(mouse, L_.canvas);

    if (drag_ == Drag::None && inCanvas) {
        const int cell = gridView_.cellAt(m, mouse);
        if (cell >= 0 && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (cell == s.start) {
                drag_ = Drag::Start;
            } else if (cell == s.goal) {
                drag_ = Drag::Goal;
            } else {
                drag_ = Drag::Paint;
                paintValue_ = brush_ == Brush::Wall ? GridMap::Cell::Wall
                              : brush_ == Brush::Mud ? GridMap::Cell::Mud
                                                     : GridMap::Cell::Empty;
                // 在已經是牆的格子上開始拖 = 擦掉（跟大多數畫圖工具一樣）
                if (m.at(cell) == paintValue_) paintValue_ = GridMap::Cell::Empty;
            }
            lastCell_ = cell;
        } else if (cell >= 0 && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            drag_ = Drag::Paint;
            paintValue_ = GridMap::Cell::Empty;
            lastCell_ = cell;
        }
    }
    if (drag_ != Drag::Start && drag_ != Drag::Goal && drag_ != Drag::Paint) return;
    if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT) && !IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
        drag_ = Drag::None;
        return;
    }

    const int cell = gridView_.cellAtClamped(m, mouse);
    bool changed = false;
    if (drag_ == Drag::Paint) {
        paintLine(lastCell_, cell, changed);
    } else {
        int& marker = drag_ == Drag::Start ? s.start : s.goal;
        const int other = drag_ == Drag::Start ? s.goal : s.start;
        if (cell != marker && cell != other && m.at(cell) != GridMap::Cell::Wall) {
            marker = cell;
            changed = true;
        }
    }
    lastCell_ = cell;
    if (changed) {
        s.rebuild();
        rerun(true);
    }
}

// 0 = 清空、1 = 隨機障礙、2 = 迷宮、3 = 還原預設
void App::gridCommand(int which) {
    Scene& s = scene();
    switch (which) {
        case 0: s.grid->clear(); break;
        case 1: s.grid->randomObstacles(seed_++, s.start, s.goal); break;
        case 2: s.grid->maze(seed_++, s.start, s.goal); break;
        case 3: s = makeScene(SceneKind::Grid); break;
    }
    s.rebuild();
    rerun(false);
}

// ---------------------------------------------------------------------------
// 畫面
// ---------------------------------------------------------------------------

void App::prepare() {
    fonts_.refresh();  // 要在 BeginDrawing 之前，換字型貼圖才安全
    L_ = layout();
    if (scene().grid) gridView_ = fitGrid(*scene().grid, L_.canvas);
    else if (scene().array.empty()) graphView_ = fitGraph(scene().graph, L_.graphArea);
}

void App::draw() {
    ClearBackground(color::Background);
    drawCanvas();
    drawPanel(trace_, player_.state(), algo_, player_.index(), player_.count(), L_.panel);
    drawTopBar();
    drawAlgoBar();
    drawSceneBar();
    drawBottomBar();
}

void App::frame() {
    ui::beginFrame();
    prepare();
    handleKeys();
    if (drag_ != Drag::Seek) {
        if (scene().grid) handleGridMouse();
        else if (scene().array.empty()) handleGraphMouse();
    }
    updatePlayback();

    BeginDrawing();
    draw();
    EndDrawing();
    ui::endFrame();
}

void App::showcase(Algo a, SceneKind k, double progress) {
    algo_ = a;
    sceneKind_ = k;
    rerun(false);
    playing_ = false;
    player_.seek(static_cast<int>(std::lround(progress * player_.count())));
}

void App::drawCanvas() {
    DrawRectangleRec(L_.canvas, color::Canvas);
    const Scene& s = scene();
    const VisualState& v = player_.state();
    BeginScissorMode(int(L_.canvas.x), int(L_.canvas.y), int(L_.canvas.width), int(L_.canvas.height));
    LegendKind legend = LegendKind::Graph;
    if (s.grid) {
        drawGrid(s, v, gridView_, markers());
        legend = LegendKind::Grid;
    } else if (!v.array.empty()) {
        drawArray(v, L_.canvas);
        legend = LegendKind::Array;
    } else {
        drawGraph(s, v, graphView_, markers());
        if (s.tree) legend = LegendKind::Tree;
    }
    if (v.matrixSize > 0) {
        DrawLineEx({L_.matrixArea.x, L_.matrixArea.y + 16}, {L_.matrixArea.x, L_.matrixArea.y + L_.matrixArea.height - 16}, 1,
                   color::Border);
        drawMatrix(s.graph, v, L_.matrixArea);
    }
    float y = L_.canvas.y + 10;
    for (const auto& line : fonts_.wrap(s.note, size::Small, L_.canvas.width - 32)) {
        text(line, L_.canvas.x + 16, y, size::Small, color::Dim);
        y += 19;
    }
    drawLegend(L_.canvas, legend);
    EndScissorMode();
}

void App::drawTopBar() {
    DrawRectangleRec(L_.top, color::Background);
    const std::string title = "演算法動畫";
    text(title, 16, 13, size::Title, color::Text);
    float x = 16 + textWidth(title, size::Title) + 28;
    const auto& cats = categories();
    const int current = categoryOf(algo_);
    for (int c = 0; c < static_cast<int>(cats.size()); ++c) {
        const float w = buttonWidth(cats[c].name) + 12;
        if (button({x, 9, w, 34}, cats[c].name, c == current)) selectAlgo(cats[c].algos.front());
        x += w + 6;
    }
}

void App::drawAlgoBar() {
    const Rectangle r = L_.algoBar;
    DrawRectangleRec(r, color::Background);
    const float y = r.y + 5, h = 32;
    text("演算法", 16, y + 7, size::Body, color::Dim);
    float x = 16 + textWidth("演算法", size::Body) + 12;
    const auto& algos = categories()[categoryOf(algo_)].algos;
    for (std::size_t i = 0; i < algos.size(); ++i) {
        const std::string label = std::format("{} {}", i + 1, algoInfo(algos[i]).shortName);
        const float w = buttonWidth(label);
        if (button({x, y, w, h}, label, algos[i] == algo_)) selectAlgo(algos[i]);
        x += w + 6;
    }
}

void App::drawSceneBar() {
    const Rectangle r = L_.sceneBar;
    DrawRectangleRec(r, color::Background);
    DrawLineEx({0, r.y + r.height}, {r.width, r.y + r.height}, 1, color::Border);
    const float y = r.y + 6, h = 32;
    text("場景", 16, y + 7, size::Body, color::Dim);
    float x = 16 + textWidth("場景", size::Body) + 12;
    for (SceneKind k : scenesFor(algo_)) {
        const std::string& name = scenes_[static_cast<int>(k)].name;
        const float w = buttonWidth(name);
        if (button({x, y, w, h}, name, k == sceneKind_)) selectScene(k);
        x += w + 6;
    }

    if (scene().grid) {
        x += 18;
        text("筆刷", x, y + 7, size::Body, color::Dim);
        x += textWidth("筆刷", size::Body) + 12;
        const std::pair<Brush, const char*> brushes[] = {{Brush::Wall, "牆"}, {Brush::Mud, "泥地"}, {Brush::Erase, "橡皮擦"}};
        for (const auto& [b, label] : brushes) {
            const float w = buttonWidth(label);
            if (button({x, y, w, h}, label, brush_ == b)) brush_ = b;
            x += w + 6;
        }
        x += 18;
        const char* commands[] = {"清空 (C)", "隨機障礙 (N)", "迷宮 (M)", "還原"};
        for (int i = 0; i < 4; ++i) {
            const float w = buttonWidth(commands[i]);
            if (button({x, y, w, h}, commands[i])) gridCommand(i);
            x += w + 6;
        }
    } else if (scene().reshufflable) {
        x += 18;
        const std::string label = "重新產生 (N)";
        const float w = buttonWidth(label);
        if (button({x, y, w, h}, label)) reshuffle();
        x += w + 6;
    }

    const std::string hint = "空白鍵 播放／暫停  ·  ←→ 單步  ·  ↑↓ 速度  ·  [ ] 類別  ·  1~9 演算法  ·  Tab 場景";
    const float hw = textWidth(hint, size::Small);
    if (r.width - hw - 16 > x + 20) text(hint, r.width - hw - 16, y + 8, size::Small, color::Dim);
}

void App::drawBottomBar() {
    const Rectangle r = L_.bottom;
    DrawRectangleRec(r, color::Background);
    DrawLineEx({0, r.y}, {r.width, r.y}, 1, color::Border);
    const float y = r.y + 13, h = 34;
    float x = 16;
    auto btn = [&](const std::string& label, float w, bool active = false) {
        const bool clicked = button({x, y, w, h}, label, active);
        x += w + 6;
        return clicked;
    };
    if (btn("開頭", 56)) stepBy(-player_.count());
    if (btn("上一步", 70)) stepBy(-1);
    if (btn(playing_ ? "暫停" : "播放", 76, playing_)) togglePlay();
    if (btn("下一步", 70)) stepBy(1);
    if (btn("結尾", 56)) stepBy(player_.count());

    x += 14;
    if (btn("-", 34)) setSpeed(speedIdx_ - 1);
    const std::string sp = speedText(Speeds[speedIdx_]);
    textCentered(sp, {x + 50, y + h / 2}, size::Body, color::Text);
    x += 100;
    if (btn("+", 34)) setSpeed(speedIdx_ + 1);

    // 進度條：點擊或拖曳可以跳到任一步
    const Rectangle p = L_.progress;
    const Rectangle hit{p.x, p.y - 12, p.width, p.height + 24};
    const Vector2 mouse = GetMousePosition();
    const bool hover = CheckCollisionPointRec(mouse, hit);
    if (hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) drag_ = Drag::Seek;
    if (drag_ == Drag::Seek) {
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            const float t = std::clamp((mouse.x - p.x) / p.width, 0.f, 1.f);
            playing_ = false;
            player_.seek(static_cast<int>(std::lround(t * player_.count())));
        } else {
            drag_ = Drag::None;
        }
    }
    const float frac = player_.count() ? float(player_.index()) / float(player_.count()) : 0;
    DrawRectangleRounded(p, 1, 6, color::PanelLight);
    DrawRectangleRounded({p.x, p.y, std::max(p.height, p.width * frac), p.height}, 1, 6, color::Accent);
    DrawCircleV({p.x + p.width * frac, p.y + p.height / 2}, hover || drag_ == Drag::Seek ? 9 : 7, color::Text);
    text(std::format("{} / {}", player_.index(), player_.count()), p.x + p.width + 16, y + 8, size::Body, color::Dim);
}

}  // namespace gav
