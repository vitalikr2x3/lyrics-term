#pragma once
// 8x8 glyphs: a character is rasterized from a TrueType font and reduced to 8 pixel rows ('#' = ink).
// The vertical scale comes from the cap height, so letters, dashes and commas keep their proportions.

#include <ft2build.h>
#include FT_FREETYPE_H

#include <algorithm>
#include <map>
#include <string>
#include <vector>

namespace glyphs8 {

// Rows (and columns of a full-height glyph) per character; --glyph-size sets it
inline int g_size = 8;

struct State {
    FT_Library lib = nullptr;
    FT_Face face = nullptr;
    std::map<char32_t, std::vector<std::string>> cache;
};

inline State& state() {
    static State s;
    return s;
}

inline void set_size(int n) {
    g_size = n < 6 ? 6 : (n > 16 ? 16 : n);
    state().cache.clear();
}

// Font size the glyphs are rasterized at, in pixels per em
constexpr int kPixelSize = 32;
// Cap height of a typical monospace font, as a fraction of the em
constexpr double kCapRatio = 0.70;

// Opens the font. False if the file cannot be read or is not a font
inline bool load(const std::string& path) {
    State& s = state();
    if (!s.lib && FT_Init_FreeType(&s.lib) != 0) return false;
    if (s.face) FT_Done_Face(s.face);
    s.face = nullptr;
    s.cache.clear();
    if (FT_New_Face(s.lib, path.c_str(), 0, &s.face) != 0) return false;
    FT_Set_Pixel_Sizes(s.face, 0, kPixelSize);
    return true;
}

// The 8 rows of one character. Spaces and characters missing from the font are blank
inline const std::vector<std::string>& rows(char32_t c) {
    State& s = state();
    auto it = s.cache.find(c);
    if (it != s.cache.end()) return it->second;

    std::vector<std::string> out(static_cast<size_t>(g_size), "   ");
    if (s.face && FT_Load_Char(s.face, static_cast<FT_ULong>(c), FT_LOAD_RENDER) == 0) {
        const FT_GlyphSlot g = s.face->glyph;
        const FT_Bitmap& bm = g->bitmap;
        const double cap = kPixelSize * kCapRatio;
        const int w = static_cast<int>(bm.width);
        const int h = static_cast<int>(bm.rows);
        if (w > 0 && h > 0) {
            const int tw = std::max(1, static_cast<int>(w * g_size / cap + 0.5));
            out.assign(static_cast<size_t>(g_size), std::string(static_cast<size_t>(tw), ' '));
            for (int r = 0; r < h; ++r) {
                // Height above the baseline of the middle of this source row
                const double y = g->bitmap_top - r - 0.5;
                const int i = static_cast<int>((cap - y) * g_size / cap);
                if (i < 0 || i >= g_size) continue;
                for (int x = 0; x < w; ++x) {
                    const int j = x * tw / w;
                    if (bm.buffer[r * bm.pitch + x] >= 140) out[i][j] = '#';
                }
            }
        }
    }
    return s.cache.emplace(c, out).first->second;
}

}  // namespace glyphs8
