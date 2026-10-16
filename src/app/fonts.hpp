#pragma once

#include <map>
#include <string>
#include <unordered_set>
#include <vector>

#include "raylib.h"

namespace gav {

// 中文字型管理。
//
// raylib 要先把「會用到的字」烘成一張貼圖才能畫，中文字太多不可能全烘。
// 所以這裡記下畫過哪些字，遇到新字就在下一幀開頭整批重建字型。
class FontBank {
public:
    bool load();  // 找系統裡的中文字型；找不到就用 raylib 內建字型（中文會變問號）
    void unload();

    void require(const std::string& text);  // 確保這些字之後畫得出來
    void refresh();                         // 每幀開頭、BeginDrawing 之前呼叫

    const Font& get(int size);
    void draw(const std::string& text, Vector2 pos, int size, Color color);
    float width(const std::string& text, int size);
    std::vector<std::string> wrap(const std::string& text, int size, float maxWidth);

    const std::string& source() const { return source_; }

private:
    Font build(int size);

    std::vector<unsigned char> data_;
    std::string source_;
    std::vector<int> codepoints_;
    std::unordered_set<int> known_;
    bool dirty_ = false;
    std::map<int, Font> fonts_;
};

}  // namespace gav
