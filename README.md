# GraphAlgoViz — 演算法動畫

用 C++ 實作圖論、最短路徑、樹的走訪與排序演算法，搭配 raylib 動畫一步步呈現演算法在做什麼。
每一步都有中文說明、虛擬碼高亮、資料結構（佇列／堆疊／優先佇列）的即時內容。

| 類別 | 演算法 | 場景 |
|---|---|---|
| 走訪 | BFS、DFS | 城市路網、格子地圖、先修課程 |
| 單源最短路 | Dijkstra、A*、Bellman-Ford | 城市路網、格子地圖、負權重、負環 |
| 全點對最短路 | Floyd-Warshall（含距離矩陣動畫） | 負權重、負環、城市路網 |
| 最小生成樹 | Prim、Kruskal | 城市路網 |
| 其他圖論 | 拓撲排序（Kahn） | 先修課程 |
| 樹的走訪 | 前序、中序、後序、層序 | 二元搜尋樹、運算式樹 |
| 排序 | 泡沫、選擇、插入、合併、快速、堆積 | 隨機、幾乎排好、由大到小、很多重複、大量（60 個） |

值得試試的比較：

- **A\* vs Dijkstra**：格子地圖上兩者路徑一樣長，但 A* 展開的格子少很多。
- **BFS vs Dijkstra**：格子地圖有泥地（成本 5），BFS 會直接穿過泥地，Dijkstra 會繞路。
- **Dijkstra 遇到負權重**：「負權重」場景裡 Dijkstra 會算錯（S→E 算成 6，正確是 5），畫面會指出出錯的那一步。
- **Bellman-Ford 的 V−1 輪**：「負權重」場景的邊故意排成最不利的順序，要跑滿 5 輪才穩定。
- **前序／中序／後序**：二元搜尋樹的中序剛好由小到大；運算式樹 (3 + 4) × (5 - 2) 的前序、中序、後序
  分別是波蘭表示法、一般寫法、逆波蘭表示法。
- **插入排序 vs 快速排序**：「幾乎排好」的資料插入排序幾乎不用動，快速排序（固定選最右邊當基準）反而退化。
- **O(n²) vs O(n log n)**：「大量（60 個）」場景，比較右下角的總步數和右側的比較次數。

## 建置

需要 CMake ≥ 3.20、支援 C++20 的編譯器（測試過 MinGW-w64 g++ 16）。
第一次設定時會從 GitHub 下載 raylib 5.5 到 `build/_deps/`，不會裝到系統裡。

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
.\build\graph_tests.exe      # 演算法測試
.\build\graph_viz.exe        # 動畫程式
```

只想建演算法和測試、不要下載 raylib：加 `-DGAV_BUILD_APP=OFF`。

> 在 Git Bash 裡跑 CMake 時 MinGW 的 `cc.exe` 可能會失敗（PATH 問題），改用 PowerShell 或 cmd。

中文字型預設讀 `C:\Windows\Fonts\msjh.ttc`（微軟正黑體）。要換字型可以設環境變數
`GAV_FONT=<字型路徑>`，或放一個 `assets/font.ttf` 在工作目錄下。

## 操作

| 按鍵／滑鼠 | 功能 |
|---|---|
| 空白鍵 | 播放／暫停 |
| ← / → | 上一步／下一步（按住可連續） |
| ↑ / ↓ | 調整速度（0.5～960 步/秒） |
| Home / End | 跳到開頭／結尾 |
| [ / ] | 切換類別（圖的走訪、最短路徑、生成樹與排程、樹的走訪、排序） |
| 1～9 | 切換類別裡的演算法 |
| Tab | 切換場景 |
| N | 隨機場景重新產生資料（二元搜尋樹、陣列） |
| R | 從頭重播 |
| 拖曳進度條 | 跳到任一步 |
| **一般圖**：左鍵／右鍵點節點 | 設為起點／終點 |
| **格子地圖**：拖曳 | 用目前的筆刷畫牆或泥地（在牆上開始拖 = 擦掉）；右鍵拖曳 = 擦掉 |
| **格子地圖**：拖曳起點／終點 | 移動它們 |
| C / N / M | 清空／隨機障礙／產生迷宮 |

編輯格子地圖時會立刻重算，畫面直接顯示結果；按 Home 再按空白鍵可以看過程。

## 架構

```
src/core/      演算法核心，純 C++，不依賴 raylib
  graph        圖（鄰接串列）
  trace        Trace / Recorder / Player：動畫的資料模型
  traversal    BFS、DFS
  shortest_path Dijkstra、A*、Bellman-Ford、Floyd-Warshall
  spanning_tree Prim、Kruskal
  topo_sort    拓撲排序
  tree         二元樹（建 BST、運算式樹、排版）
  tree_traversal 前序、中序、後序、層序
  sorting      六種排序
  scenes       範例場景、格子地圖、演算法與場景的對應
src/app/       raylib 動畫程式（字型、繪圖、輸入、版面）
tests/         演算法正確性測試
tools/         check_glyphs.py：檢查說明文字用到的字字型裡有沒有
```

**演算法不碰畫面。** 每個演算法一邊跑，一邊透過 `Recorder` 記下「這一步哪些點／邊變成什麼狀態、
說明文字、對應虛擬碼第幾行」，最後得到一份 `Trace`。播放器（`Player`）再把 Trace 套到畫面狀態上。

這樣的好處：

- 可以倒退、拖進度條（每一步只存變化量，倒退時從頭重播，幾千步也是瞬間）
- 演算法可以直接寫測試，不用開視窗
- 演算法程式碼讀起來跟課本差不多，只是多了幾行 `rec.xxx()`

### 新增一個演算法

1. 在 `algorithms.hpp` 宣告，找個合適的 `.cpp` 實作。寫法參考 `bfs()`：
   ```cpp
   Recorder rec(g, "名稱", {"虛擬碼第 0 行", "第 1 行", ...}, "資料結構標題");
   rec.node(u, NodeState::Current);         // 改點的狀態
   rec.flashEdge(e, EdgeState::Examining);  // 只亮這一步
   rec.label(u, "3");                       // 點旁的小標籤
   rec.container({...});                    // 右側面板列出的資料結構內容
   rec.step(2, "這一步在做什麼");           // 存成一步，2 = 虛擬碼行號
   return rec.finish(std::move(result));
   ```
   排序用 `rec.initArray()`、`rec.value()`、`rec.bar()`、`rec.pointers()`，可以參考 `sorting.cpp` 的 `SortRecorder`。
2. 在 `scenes.hpp` 的 `Algo` 加一項，`scenes.cpp` 補 `algoInfo`（順序要跟 enum 一致）、`categories`、
   `scenesFor`、`runAlgorithm`。
3. 在 `tests/test_algorithms.cpp` 加測試（`traces_replay_consistently` 會自動涵蓋新演算法的重播檢查）。

### 字型缺字

raylib 遇到字型裡沒有的字會直接畫成空白，不會報錯。微軟正黑體就沒有 `−`（U+2212）、`≤`、`≥` 和全形空白，
所以程式裡一律用 `-`、`<=`、`>=`。新增說明文字後跑一下：

```powershell
python tools/check_glyphs.py      # 需要 pip install fonttools
```

### 截圖模式

```powershell
.\build\graph_viz.exe --screenshot <資料夾>
```

視窗不會顯示，會把幾個代表性畫面存成 PNG，用來檢查版面。
