#include "app/fonts.hpp"

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iterator>

namespace gav {

namespace {

constexpr float Spacing = 1;

std::uint32_t be32(const unsigned char* p) {
    return (std::uint32_t(p[0]) << 24) | (std::uint32_t(p[1]) << 16) | (std::uint32_t(p[2]) << 8) | p[3];
}
std::uint16_t be16(const unsigned char* p) { return std::uint16_t((p[0] << 8) | p[1]); }

// raylib 只吃 .ttf/.otf。Windows 的中文字型多半是 .ttc（好幾個字型包在同一個檔）。
// .ttc 裡每個字型的「表格目錄」記的位移本來就是相對於檔頭，
// 所以把第 0 個字型的表格目錄搬到檔頭，整個檔案就是一個合法的 .ttf。
bool ttcToTtf(std::vector<unsigned char>& d) {
    if (d.size() < 16 || std::memcmp(d.data(), "ttcf", 4) != 0) return false;
    const std::uint32_t off = be32(&d[12]);
    if (off + 12 > d.size()) return false;
    const std::size_t len = 12 + 16 * std::size_t(be16(&d[off + 4]));
    if (off + len > d.size()) return false;
    // 確認表格資料都在要覆蓋的範圍之後，才不會被蓋掉
    for (std::size_t i = off + 12; i < off + len; i += 16)
        if (be32(&d[i + 8]) < len) return false;
    std::memmove(d.data(), d.data() + off, len);
    return true;
}

bool readFile(const std::string& path, std::vector<unsigned char>& out) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    out.assign(std::istreambuf_iterator<char>(f), {});
    return !out.empty();
}

std::vector<std::string> candidates() {
    std::vector<std::string> list;
    if (const char* env = std::getenv("GAV_FONT")) list.emplace_back(env);
    list.emplace_back("assets/font.ttf");
    const char* win = std::getenv("WINDIR");
    const std::string fonts = std::string(win ? win : "C:/Windows") + "/Fonts/";
    for (const char* name : {"msjh.ttc", "kaiu.ttf", "mingliu.ttc", "NotoSansTC-VF.ttf", "msyh.ttc"})
        list.push_back(fonts + name);
    list.emplace_back("/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc");
    list.emplace_back("/usr/share/fonts/noto-cjk/NotoSansCJK-Regular.ttc");
    list.emplace_back("/System/Library/Fonts/PingFang.ttc");
    return list;
}

}  // namespace

bool FontBank::load() {
    for (int cp = 32; cp < 127; ++cp) {
        codepoints_.push_back(cp);
        known_.insert(cp);
    }
    for (const std::string& path : candidates()) {
        std::vector<unsigned char> d;
        if (!readFile(path, d)) continue;
        if (std::memcmp(d.data(), "ttcf", 4) == 0 && !ttcToTtf(d)) continue;
        data_ = std::move(d);
        source_ = path;
        return true;
    }
    return false;
}

void FontBank::unload() {
    for (auto& [size, font] : fonts_)
        if (font.texture.id != GetFontDefault().texture.id) UnloadFont(font);
    fonts_.clear();
}

void FontBank::require(const std::string& text) {
    const char* p = text.c_str();
    while (*p) {
        int bytes = 0;
        const int cp = GetCodepointNext(p, &bytes);
        p += bytes;
        if (known_.insert(cp).second) {
            codepoints_.push_back(cp);
            dirty_ = true;
        }
    }
}

void FontBank::refresh() {
    if (!dirty_) return;
    dirty_ = false;
    std::vector<int> sizes;
    for (const auto& [size, font] : fonts_) sizes.push_back(size);
    unload();
    for (int size : sizes) fonts_[size] = build(size);
}

Font FontBank::build(int size) {
    if (data_.empty()) return GetFontDefault();
    Font f = LoadFontFromMemory(".ttf", data_.data(), static_cast<int>(data_.size()), size, codepoints_.data(),
                                static_cast<int>(codepoints_.size()));
    if (f.texture.id == 0) return GetFontDefault();
    SetTextureFilter(f.texture, TEXTURE_FILTER_BILINEAR);
    return f;
}

const Font& FontBank::get(int size) {
    auto it = fonts_.find(size);
    if (it == fonts_.end()) it = fonts_.emplace(size, build(size)).first;
    return it->second;
}

void FontBank::draw(const std::string& text, Vector2 pos, int size, Color color) {
    require(text);
    DrawTextEx(get(size), text.c_str(), {std::round(pos.x), std::round(pos.y)}, float(size), Spacing, color);
}

float FontBank::width(const std::string& text, int size) {
    require(text);
    return MeasureTextEx(get(size), text.c_str(), float(size), Spacing).x;
}

std::vector<std::string> FontBank::wrap(const std::string& text, int size, float maxWidth) {
    require(text);
    const Font& font = get(size);
    const float scale = float(size) / float(font.baseSize);
    auto advance = [&](int cp) {
        const int i = GetGlyphIndex(font, cp);
        const float a = font.glyphs[i].advanceX != 0 ? float(font.glyphs[i].advanceX) : font.recs[i].width;
        return a * scale + Spacing;
    };
    // 英數字連在一起的不從中間斷開，中文則每個字之間都可以斷
    auto isWordChar = [](int cp) {
        return cp < 128 && ((cp >= '0' && cp <= '9') || (cp >= 'a' && cp <= 'z') || (cp >= 'A' && cp <= 'Z') ||
                            cp == '_' || cp == '[' || cp == ']' || cp == '.' || cp == '(' || cp == ')');
    };

    std::vector<std::string> lines;
    std::size_t lineStart = 0, lastBreak = std::string::npos;
    float w = 0, widthAtBreak = 0;
    std::size_t i = 0;
    while (i < text.size()) {
        int bytes = 0;
        const int cp = GetCodepointNext(text.c_str() + i, &bytes);
        if (cp == '\n') {
            lines.push_back(text.substr(lineStart, i - lineStart));
            i += bytes;
            lineStart = i;
            lastBreak = std::string::npos;
            w = 0;
            continue;
        }
        const float a = advance(cp);
        if (w + a > maxWidth && i > lineStart) {
            if (lastBreak != std::string::npos && lastBreak > lineStart) {
                lines.push_back(text.substr(lineStart, lastBreak - lineStart));
                lineStart = lastBreak;
                w -= widthAtBreak;
            } else {
                lines.push_back(text.substr(lineStart, i - lineStart));
                lineStart = i;
                w = 0;
            }
            lastBreak = std::string::npos;
        }
        w += a;
        i += bytes;
        if (!isWordChar(cp)) {
            lastBreak = i;
            widthAtBreak = w;
        }
    }
    if (lineStart < text.size()) lines.push_back(text.substr(lineStart));
    return lines;
}

}  // namespace gav
