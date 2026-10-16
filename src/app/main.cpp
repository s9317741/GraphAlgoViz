#include <algorithm>
#include <cstring>
#include <string>

#include "app/app.hpp"
#include "raylib.h"

namespace {

// 截圖模式：視窗不顯示，畫到離屏貼圖再存成 PNG。用來檢查版面，不會跳出視窗。
//   graph_viz --screenshot <資料夾>
int screenshots(const std::string& dir) {
    using gav::Algo;
    using gav::SceneKind;
    constexpr int W = 1480, H = 900;
    SetConfigFlags(FLAG_WINDOW_HIDDEN);
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(W, H, "screenshot");
    {
        gav::App app;
        RenderTexture2D rt = LoadRenderTexture(W, H);
        struct Shot {
            Algo algo;
            SceneKind scene;
            double progress;
            const char* name;
        };
        const Shot shots[] = {
            {Algo::Dijkstra, SceneKind::City, 0.45, "dijkstra_city"},
            {Algo::AStar, SceneKind::Grid, 1.0, "astar_grid"},
            {Algo::Dijkstra, SceneKind::Grid, 1.0, "dijkstra_grid"},
            {Algo::FloydWarshall, SceneKind::Negative, 0.5, "floyd_negative"},
            {Algo::BellmanFord, SceneKind::NegativeCycle, 1.0, "bellman_cycle"},
            {Algo::Prim, SceneKind::City, 0.6, "prim_city"},
            {Algo::Kruskal, SceneKind::City, 1.0, "kruskal_city"},
            {Algo::TopoSort, SceneKind::Dag, 0.5, "topo_dag"},
            {Algo::DFS, SceneKind::City, 0.4, "dfs_city"},
            {Algo::InOrder, SceneKind::Bst, 0.55, "inorder_bst"},
            {Algo::PostOrder, SceneKind::Expression, 1.0, "postorder_expr"},
            {Algo::LevelOrder, SceneKind::Bst, 0.5, "levelorder_bst"},
            {Algo::Quick, SceneKind::ArrayRandom, 0.3, "quick_random"},
            {Algo::Merge, SceneKind::ArrayLarge, 0.6, "merge_large"},
            {Algo::Heap, SceneKind::ArrayFewUnique, 0.5, "heap_few"},
            {Algo::Insertion, SceneKind::ArrayNearlySorted, 1.0, "insertion_nearly"},
        };
        for (const Shot& s : shots) {
            app.showcase(s.algo, s.scene, s.progress);
            // 第一次畫會發現新字、下一輪才烘進字型，所以畫兩次
            for (int pass = 0; pass < 2; ++pass) {
                app.prepare();
                BeginTextureMode(rt);
                app.draw();
                EndTextureMode();
            }
            Image img = LoadImageFromTexture(rt.texture);
            ImageFlipVertical(&img);
            ExportImage(img, (dir + "/" + s.name + ".png").c_str());
            UnloadImage(img);
        }
        UnloadRenderTexture(rt);
    }
    CloseWindow();
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc >= 3 && std::strcmp(argv[1], "--screenshot") == 0) return screenshots(argv[2]);

    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(1480, 900, "圖論演算法動畫");

    // 螢幕比預設視窗小時（例如筆電），縮到螢幕的九成
    const int monitor = GetCurrentMonitor();
    const int mw = GetMonitorWidth(monitor), mh = GetMonitorHeight(monitor);
    if (mw > 0 && mh > 0 && (mw < 1520 || mh < 960)) {
        const int w = std::min(1480, mw * 9 / 10), h = std::min(900, mh * 85 / 100);
        SetWindowSize(w, h);
        SetWindowPosition((mw - w) / 2, (mh - h) / 2);
    }
    SetWindowMinSize(1200, 720);
    SetTargetFPS(60);
    SetExitKey(KEY_NULL);  // Esc 不要直接關掉視窗

    {
        gav::App app;  // 字型貼圖要在 CloseWindow 之前釋放
        while (!WindowShouldClose()) app.frame();
    }
    CloseWindow();
    return 0;
}
