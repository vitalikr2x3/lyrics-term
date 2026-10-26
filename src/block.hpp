#pragma once
// Block renderer: text drawn with the block font, word wrapping and alignment.

#include <algorithm>
#include <cstdio>
#include <sstream>
#include <string>
#include <vector>

#include "glyphs.hpp"
#include "util.hpp"

namespace block {

// 5 ASCII rows ('#' and ' ') for the text. Each glyph has a one-cell gap on the right
inline std::vector<std::string> text_to_rows(const std::string& text) {
    std::vector<std::string> rows(5);
    for (char32_t cp : util::decode_utf8(text)) {
        char32_t c = util::to_upper(cp);
        const Glyph* g = find_glyph(c);
        size_t w = 5;
        if (g) {
            w = 0;
            for (auto r : g->rows) w = std::max(w, r.size());
        }
        for (int i = 0; i < 5; ++i) {
            std::string part = g ? std::string(g->rows[i]) : std::string(5, ' ');
            part.resize(w, ' ');
            rows[i] += part + " ";
        }
    }
    return rows;
}

inline int text_width(const std::string& text) {
    return static_cast<int>(text_to_rows(text)[0].size());
}

// Splits text into lines that fit the terminal. Long words are split by characters
inline std::vector<std::string> wrap_words(const std::string& text, int cols) {
    std::vector<std::string> out;
    std::string cur;
    std::string word;
    std::istringstream ss(text);

    while (ss >> word) {
        if (text_width(word) > cols) {
            if (!cur.empty()) {
                out.push_back(cur);
                cur.clear();
            }
            std::string piece;
            for (char32_t c : util::decode_utf8(word)) {
                std::string next = piece + util::encode_utf8(c);
                if (text_width(next) > cols && !piece.empty()) {
                    out.push_back(piece);
                    next = util::encode_utf8(c);
                }
                piece = next;
            }
            word = piece;
        }
        std::string cand = cur.empty() ? word : cur + " " + word;
        if (text_width(cand) <= cols) {
            cur = cand;
        } else {
            out.push_back(cur);
            cur = word;
        }
    }
    if (!cur.empty()) out.push_back(cur);
    return out;
}

// Characters missing from the font, shown as a hint under the line
inline std::string missing_note(const std::string& text) {
    std::string miss;
    for (char32_t cp : util::decode_utf8(text)) {
        char32_t c = util::to_upper(cp);
        if (c == U' ' || find_glyph(c)) continue;
        std::string s = util::encode_utf8(c);
        if (miss.find(s) == std::string::npos) miss += s;
    }
    if (miss.empty()) return "";
    return "missing in font: " + miss;
}

inline std::string to_block_chars(const std::string& row) {
    std::string out;
    out.reserve(row.size() * 3);
    for (char c : row) out += (c == '#') ? "█" : " ";
    return out;
}

// Draws the header, the previous line (dim), the block text, a hint and the next line (dim)
inline void draw(const std::string& text, const std::string& header, const std::string& note,
                 const std::string& prev, const std::string& next, util::Rgb rgb, bool bold, bool dim,
                 bool left) {
    util::TermSize t = util::term_size();

    std::string style = "\033[38;2;" + std::to_string(rgb.r) + ";" + std::to_string(rgb.g) + ";" +
                        std::to_string(rgb.b) + "m";
    if (bold) style += "\033[1m";
    if (dim) style += "\033[2m";

    struct Line {
        std::string text;
        int width;
    };
    std::vector<Line> body;
    auto chunks = wrap_words(text, t.cols);
    for (size_t n = 0; n < chunks.size(); ++n) {
        if (n) body.push_back({"", 0});
        for (const auto& row : text_to_rows(chunks[n]))
            body.push_back({style + to_block_chars(row) + "\033[0m", static_cast<int>(row.size())});
    }

    int height = static_cast<int>(body.size()) + (header.empty() ? 0 : 2) + (prev.empty() ? 0 : 2) +
                 (note.empty() ? 0 : 1) + (next.empty() ? 0 : 2);
    int top = util::top_row(t.rows, height);

    auto pad = [&](int width) { return std::string(left ? 2 : std::max(0, (t.cols - width) / 2), ' '); };
    auto cw = [](const std::string& s) { return static_cast<int>(util::codepoints(s)); };

    std::string out = "\033[2J\033[H" + std::string(top, '\n');
    if (!header.empty()) out += pad(cw(header)) + "\033[2m" + header + "\033[0m\n\n";
    if (!prev.empty()) out += pad(cw(prev)) + "\033[2m" + prev + "\033[0m\n\n";
    for (const auto& line : body) out += pad(line.width) + line.text + "\n";
    if (!note.empty()) out += pad(cw(note)) + "\033[2m" + note + "\033[0m\n";
    if (!next.empty()) out += "\n" + pad(cw(next)) + "\033[2m" + next + "\033[0m\n";

    std::fwrite(out.data(), 1, out.size(), stdout);
    std::fflush(stdout);
}

}  // namespace block
