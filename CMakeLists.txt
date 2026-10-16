cmake_minimum_required(VERSION 3.20)
project(GraphAlgoViz LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

option(GAV_BUILD_APP "建置 raylib 動畫程式（第一次會從 GitHub 下載 raylib）" ON)

# ---- 演算法核心：純 C++，不依賴任何圖形函式庫 ----
add_library(graphcore STATIC
  src/core/graph.cpp
  src/core/trace.cpp
  src/core/algo_common.cpp
  src/core/traversal.cpp
  src/core/shortest_path.cpp
  src/core/spanning_tree.cpp
  src/core/topo_sort.cpp
  src/core/tree.cpp
  src/core/tree_traversal.cpp
  src/core/sorting.cpp
  src/core/scenes.cpp
)
target_include_directories(graphcore PUBLIC src)
if(MSVC)
  target_compile_options(graphcore PRIVATE /W4 /utf-8)
else()
  target_compile_options(graphcore PRIVATE -Wall -Wextra)
endif()

# MinGW 靜態連結，exe 拿到別台 Windows 也能直接跑（不需要 msys2 的 DLL）
function(gav_static_runtime target)
  if(MINGW)
    target_link_options(${target} PRIVATE -static)
  endif()
endfunction()

# ---- 測試 ----
enable_testing()
add_executable(graph_tests tests/test_algorithms.cpp)
target_link_libraries(graph_tests PRIVATE graphcore)
gav_static_runtime(graph_tests)
add_test(NAME graph_tests COMMAND graph_tests)

# ---- 動畫程式 ----
if(GAV_BUILD_APP)
  include(FetchContent)
  set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(raylib
    URL https://github.com/raysan5/raylib/archive/refs/tags/5.5.tar.gz
    URL_HASH SHA256=aea98ecf5bc5c5e0b789a76de0083a21a70457050ea4cc2aec7566935f5e258e
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  )
  FetchContent_MakeAvailable(raylib)

  add_executable(graph_viz
    src/app/main.cpp
    src/app/fonts.cpp
    src/app/ui.cpp
    src/app/render.cpp
    src/app/app.cpp
  )
  target_link_libraries(graph_viz PRIVATE graphcore raylib)
  gav_static_runtime(graph_viz)
  if(WIN32 AND CMAKE_BUILD_TYPE STREQUAL "Release")
    set_target_properties(graph_viz PROPERTIES WIN32_EXECUTABLE ON)  # 不要多開一個黑色主控台視窗
  endif()
endif()
