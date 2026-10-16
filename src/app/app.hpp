#pragma once

#include <array>

#include "app/fonts.hpp"
#include "app/render.hpp"
#include "core/scenes.hpp"
#include "core/trace.hpp"
#include "raylib.h"

namespace gav {

class App {
public:
    App();
    ~App();
    App(const App&) = delete;
    App& operator=(const App&) = delete;

    void frame();  // 處理輸入、推進動畫、畫一幀

    // 截圖模式用：直接切到某個演算法／場景的某個進度（0~1），然後只畫不處理輸入
    void showcase(Algo a, SceneKind k, double progress);
    void prepare();
    void draw();

private:
    enum class Brush { Wall, Mud, Erase };
    enum class Drag { None, Start, Goal, Paint, Seek };

    struct Layout {
        Rectangle top, algoBar, sceneBar, canvas, graphArea, matrixArea, panel, bottom, progress;
    };

    Scene& scene() { return scenes_[static_cast<int>(sceneKind_)]; }
    Markers markers() const;
    Layout layout() const;

    void selectAlgo(Algo a);
    void selectScene(SceneKind k);
    void rerun(bool showEnd);  // 重新執行演算法；showEnd = 直接跳到結果（編輯地圖時）
    void prewarmFonts();
    void requireTrace();

    void togglePlay();
    void stepBy(int delta);
    void setSpeed(int idx);
    void updatePlayback();

    void handleKeys();
    void handleGraphMouse();
    void handleGridMouse();
    void paintLine(int from, int to, bool& changed);
    void gridCommand(int which);
    void reshuffle();  // 隨機場景重新產生資料

    void drawTopBar();
    void drawAlgoBar();
    void drawSceneBar();
    void drawBottomBar();
    void drawCanvas();

    FontBank fonts_;
    std::array<Scene, SceneCount> scenes_;
    Algo algo_ = Algo::Dijkstra;
    SceneKind sceneKind_ = SceneKind::City;
    Trace trace_;
    Player player_;

    bool playing_ = false;
    int speedIdx_ = 2;
    double acc_ = 0;

    Brush brush_ = Brush::Wall;
    Drag drag_ = Drag::None;
    GridMap::Cell paintValue_ = GridMap::Cell::Wall;
    int lastCell_ = -1;
    unsigned seed_ = 1;

    Layout L_{};
    GraphView graphView_;
    GridView gridView_;
};

}  // namespace gav
