#pragma once
// Graphics renderer: text is drawn with FreeType into an RGBA buffer and sent with the kitty graphics protocol.

#include <ft2build.h>
#include FT_FREETYPE_H

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <vector>

#include "util.hpp"

namespace kitty {

struct Image {
    int w = 0, h = 0;
    std::vector<uint8_t> rgba;  // RGBA, straight (not premultiplied) alpha
};

class Renderer {
public:
    Renderer(const std::string& font_path, int size, int spacing, bool glow)
        : size_(size), spacing_(spacing), glow_(glow) {
        if (FT_Init_FreeType(&lib_)) throw std::runtime_error("FreeType failed to initialize");
        if (FT_New_Face(lib_, font_path.c_str(), 0, &face_))
            throw std::runtime_error("cannot open font: " + font_path);
        FT_Set_Pixel_Sizes(face_, 0, size_);
    }

    ~Renderer() {
        if (face_) FT_Done_Face(face_);
        if (lib_) FT_Done_FreeType(lib_);
    }

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    // opacity: 0..1 multiplier for the text (dimming while paused, fade-in)
    Image render(const std::string& text, int max_w, util::Rgb rgb, float opacity) {
        std::u32string up;
        for (char32_t c : util::decode_utf8(text)) up += util::to_upper(c);
        auto lines = wrap(up, max_w);

        int asc = static_cast<int>(face_->size->metrics.ascender / 64);
        int desc = static_cast<int>(-face_->size->metrics.descender / 64);
        int line_h = asc + desc + std::max(2, size_ / 8);
        int pad = glow_ ? size_ / 2 : std::max(2, size_ / 4);

        int text_w = 0;
        for (const auto& l : lines) text_w = std::max(text_w, measure(l));
        int w = text_w + pad * 2;
        int h = line_h * static_cast<int>(lines.size()) + pad * 2;

        std::vector<uint8_t> alpha(static_cast<size_t>(w) * static_cast<size_t>(h), 0);
        for (size_t li = 0; li < lines.size(); ++li) {
            int pen = pad;
            int base = pad + static_cast<int>(li) * line_h + asc;
            for (char32_t c : lines[li]) {
                if (FT_Load_Char(face_, c, FT_LOAD_RENDER) != 0) continue;
                FT_GlyphSlot g = face_->glyph;
                const FT_Bitmap& bm = g->bitmap;
                for (unsigned r = 0; r < bm.rows; ++r) {
                    for (unsigned k = 0; k < bm.width; ++k) {
                        int x = pen + g->bitmap_left + static_cast<int>(k);
                        int y = base - g->bitmap_top + static_cast<int>(r);
                        if (x < 0 || y < 0 || x >= w || y >= h) continue;
                        uint8_t a = bm.buffer[r * static_cast<unsigned>(bm.pitch) + k];
                        uint8_t& dst = alpha[static_cast<size_t>(y) * w + x];
                        dst = std::max(dst, a);
                    }
                }
                pen += static_cast<int>(g->advance.x / 64) + spacing_;
            }
        }

        std::vector<uint8_t> final_alpha = alpha;
        if (glow_) {
            auto halo = box_blur(alpha, w, h, std::max(1, size_ / 10));
            for (size_t i = 0; i < alpha.size(); ++i) {
                uint8_t hv = static_cast<uint8_t>(halo[i] * 0.55);
                final_alpha[i] = std::max(alpha[i], hv);
            }
        }
        if (opacity < 1.0f)
            for (auto& a : final_alpha) a = static_cast<uint8_t>(a * opacity);

        Image img;
        img.w = w;
        img.h = h;
        img.rgba.resize(static_cast<size_t>(w) * static_cast<size_t>(h) * 4);
        for (size_t i = 0; i < final_alpha.size(); ++i) {
            img.rgba[i * 4 + 0] = rgb.r;
            img.rgba[i * 4 + 1] = rgb.g;
            img.rgba[i * 4 + 2] = rgb.b;
            img.rgba[i * 4 + 3] = final_alpha[i];
        }
        return img;
    }

private:
    FT_Library lib_ = nullptr;
    FT_Face face_ = nullptr;
    int size_;
    int spacing_;
    bool glow_;

    int advance(char32_t c) {
        if (FT_Load_Char(face_, c, FT_LOAD_DEFAULT) != 0) return 0;
        return static_cast<int>(face_->glyph->advance.x / 64);
    }

    int measure(const std::u32string& s) {
        if (s.empty()) return 0;
        int total = 0;
        for (char32_t c : s) total += advance(c);
        return total + spacing_ * static_cast<int>(s.size() - 1);
    }

    // Word wrap based on the real pixel width
    std::vector<std::u32string> wrap(const std::u32string& text, int max_w) {
        std::vector<std::u32string> words;
        std::u32string w;
        for (char32_t c : text) {
            if (c == U' ') {
                if (!w.empty()) {
                    words.push_back(w);
                    w.clear();
                }
            } else {
                w += c;
            }
        }
        if (!w.empty()) words.push_back(w);

        std::vector<std::u32string> lines;
        std::u32string cur;
        for (const auto& wd : words) {
            std::u32string cand = cur.empty() ? wd : cur + U' ' + wd;
            if (cur.empty() || measure(cand) <= max_w) {
                cur = cand;
            } else {
                lines.push_back(cur);
                cur = wd;
            }
        }
        if (!cur.empty()) lines.push_back(cur);
        if (lines.empty()) lines.push_back(U"");
        return lines;
    }

    // Separable box blur of the alpha channel, used for the glow
    static std::vector<uint8_t> box_blur(const std::vector<uint8_t>& src, int w, int h, int r) {
        std::vector<uint8_t> tmp(src.size()), dst(src.size());
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                int sum = 0, n = 0;
                for (int k = -r; k <= r; ++k) {
                    int xx = x + k;
                    if (xx < 0 || xx >= w) continue;
                    sum += src[static_cast<size_t>(y) * w + xx];
                    ++n;
                }
                tmp[static_cast<size_t>(y) * w + x] = static_cast<uint8_t>(sum / n);
            }
        }
        for (int x = 0; x < w; ++x) {
            for (int y = 0; y < h; ++y) {
                int sum = 0, n = 0;
                for (int k = -r; k <= r; ++k) {
                    int yy = y + k;
                    if (yy < 0 || yy >= h) continue;
                    sum += tmp[static_cast<size_t>(yy) * w + x];
                    ++n;
                }
                dst[static_cast<size_t>(y) * w + x] = static_cast<uint8_t>(sum / n);
            }
        }
        return dst;
    }
};

inline std::string base64(const std::vector<uint8_t>& in) {
    static const char* T = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    out.reserve((in.size() + 2) / 3 * 4);
    for (size_t i = 0; i < in.size(); i += 3) {
        uint32_t v = static_cast<uint32_t>(in[i]) << 16;
        if (i + 1 < in.size()) v |= static_cast<uint32_t>(in[i + 1]) << 8;
        if (i + 2 < in.size()) v |= static_cast<uint32_t>(in[i + 2]);
        out += T[(v >> 18) & 63];
        out += T[(v >> 12) & 63];
        out += (i + 1 < in.size()) ? T[(v >> 6) & 63] : '=';
        out += (i + 2 < in.size()) ? T[v & 63] : '=';
    }
    return out;
}

// kitty graphics protocol: raw RGBA, base64 in 4096-byte chunks
inline std::string image_sequence(const Image& img) {
    std::string b64 = base64(img.rgba);
    std::string out;
    size_t pos = 0;
    bool first = true;
    while (pos < b64.size()) {
        size_t n = std::min<size_t>(4096, b64.size() - pos);
        bool more = pos + n < b64.size();
        out += "\x1b_G";
        if (first)
            out += "a=T,f=32,s=" + std::to_string(img.w) + ",v=" + std::to_string(img.h) + ",q=2,";
        out += std::string("m=") + (more ? "1" : "0") + ";";
        out.append(b64, pos, n);
        out += "\x1b\\";
        pos += n;
        first = false;
    }
    return out;
}

// Draws the header, the previous line, the image of the current line, a hint and the next line
inline void draw(Renderer& r, const std::string& text, const std::string& header, const std::string& note,
                 const std::string& prev, const std::string& next, util::Rgb rgb, float opacity, bool left) {
    util::TermSize t = util::term_size();

    Image img;
    if (!text.empty()) img = r.render(text, t.cols * t.cell_w, rgb, opacity);
    int img_rows = img.w ? (img.h + t.cell_h - 1) / t.cell_h : 0;
    int img_cols = img.w ? (img.w + t.cell_w - 1) / t.cell_w : 0;

    int height = (header.empty() ? 0 : 2) + (prev.empty() ? 0 : 2) + img_rows + (note.empty() ? 0 : 1) +
                 (next.empty() ? 0 : 2);
    int top = util::top_row(t.rows, height);

    auto at = [](int row, int col) {
        return "\033[" + std::to_string(row + 1) + ";" + std::to_string(col + 1) + "H";
    };
    auto col_for = [&](int width) { return left ? 2 : std::max(0, (t.cols - width) / 2); };
    auto cw = [](const std::string& s) { return static_cast<int>(util::codepoints(s)); };

    // Delete old kitty images, then clear the screen
    std::string out = "\x1b_Ga=d,d=a,q=2\x1b\\\033[2J\033[H";
    int row = top;
    if (!header.empty()) {
        out += at(row, col_for(cw(header))) + "\033[2m" + header + "\033[0m";
        row += 2;
    }
    if (!prev.empty()) {
        out += at(row, col_for(cw(prev))) + "\033[2m" + prev + "\033[0m";
        row += 2;
    }
    if (img.w) {
        out += at(row, col_for(img_cols)) + image_sequence(img);
        row += img_rows;
    }
    if (!note.empty()) {
        out += at(row, col_for(cw(note))) + "\033[2m" + note + "\033[0m";
        row += 1;
    }
    if (!next.empty()) out += at(row + 1, col_for(cw(next))) + "\033[2m" + next + "\033[0m";

    std::fwrite(out.data(), 1, out.size(), stdout);
    std::fflush(stdout);
}

}  // namespace kitty
