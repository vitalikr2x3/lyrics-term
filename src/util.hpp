#pragma once
// Shared helpers: UTF-8 handling, case folding, colors, terminal size, kitty palette.

#include <sys/ioctl.h>
#include <unistd.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace util {

struct Rgb {
    uint8_t r = 0, g = 0, b = 0;
};

using Palette = std::map<std::string, Rgb>;

inline std::vector<char32_t> decode_utf8(const std::string& s) {
    std::vector<char32_t> out;
    for (size_t i = 0; i < s.size();) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        size_t len = c < 0x80 ? 1 : (c >> 5) == 0x6 ? 2 : (c >> 4) == 0xE ? 3 : (c >> 3) == 0x1E ? 4 : 1;
        char32_t cp = len == 1 ? c : (c & (0xFFu >> (len + 1)));
        for (size_t k = 1; k < len && i + k < s.size(); ++k)
            cp = (cp << 6) | (static_cast<unsigned char>(s[i + k]) & 0x3F);
        out.push_back(cp);
        i += len;
    }
    return out;
}

inline std::string encode_utf8(char32_t cp) {
    std::string r;
    if (cp < 0x80) {
        r += static_cast<char>(cp);
    } else if (cp < 0x800) {
        r += static_cast<char>(0xC0 | (cp >> 6));
        r += static_cast<char>(0x80 | (cp & 0x3F));
    } else if (cp < 0x10000) {
        r += static_cast<char>(0xE0 | (cp >> 12));
        r += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        r += static_cast<char>(0x80 | (cp & 0x3F));
    } else {
        r += static_cast<char>(0xF0 | (cp >> 18));
        r += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
        r += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        r += static_cast<char>(0x80 | (cp & 0x3F));
    }
    return r;
}

inline size_t codepoints(const std::string& s) {
    return decode_utf8(s).size();
}

// Uppercase for Latin and Cyrillic (including Ukrainian letters)
inline char32_t to_upper(char32_t c) {
    if (c >= U'a' && c <= U'z') return c - 32;
    if (c >= 0x430 && c <= 0x44F) return c - 0x20;  // а-я
    if (c == 0x451) return 0x401;                    // ё
    if (c == 0x454) return 0x404;                    // є
    if (c == 0x456) return 0x406;                    // і
    if (c == 0x457) return 0x407;                    // ї
    if (c == 0x491) return 0x490;                    // ґ
    return c;
}

inline std::string lower(std::string s) {
    for (auto& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

inline std::optional<Rgb> parse_hex(std::string s) {
    if (!s.empty() && s[0] == '#') s.erase(0, 1);
    if (s.size() != 6) return std::nullopt;
    for (char c : s)
        if (!std::isxdigit(static_cast<unsigned char>(c))) return std::nullopt;
    auto byte = [&](size_t i) { return static_cast<uint8_t>(std::stoi(s.substr(i, 2), nullptr, 16)); };
    return Rgb{byte(0), byte(2), byte(4)};
}

inline std::optional<Rgb> named_color(const std::string& name) {
    static const std::map<std::string, Rgb> named = {
        {"cyan", {0, 205, 205}},   {"green", {0, 205, 0}},     {"red", {205, 0, 0}},
        {"yellow", {205, 205, 0}}, {"blue", {0, 0, 238}},      {"magenta", {205, 0, 205}},
        {"white", {229, 229, 229}},
    };
    auto it = named.find(name);
    if (it == named.end()) return std::nullopt;
    return it->second;
}

// Palette of the current kitty window: {color2: rgb, foreground: rgb, ...}
inline Palette kitty_palette() {
    Palette pal;
    FILE* p = popen("kitten @ get-colors 2>/dev/null", "r");
    if (!p) return pal;
    char buf[256];
    while (std::fgets(buf, sizeof buf, p)) {
        char name[64] = {0}, hex[16] = {0};
        if (std::sscanf(buf, "%63s %15s", name, hex) == 2) {
            if (auto c = parse_hex(hex)) pal[lower(name)] = *c;
        }
    }
    pclose(p);
    return pal;
}

// Accepts a palette slot name, a named color, or a HEX value
inline std::optional<Rgb> resolve_color(const std::string& value, const Palette& pal) {
    std::string key = lower(value);
    auto it = pal.find(key);
    if (it != pal.end()) return it->second;
    if (auto n = named_color(key)) return n;
    return parse_hex(key);
}

inline bool in_kitty() {
    if (std::getenv("KITTY_WINDOW_ID")) return true;
    const char* term = std::getenv("TERM");
    return term && std::string(term) == "xterm-kitty";
}

struct TermSize {
    int cols = 80, rows = 24;
    int cell_w = 10, cell_h = 20;  // cell size in pixels, used when the terminal does not report it
};

inline TermSize term_size() {
    TermSize t;
    winsize ws{};
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0) {
        if (ws.ws_col) t.cols = ws.ws_col;
        if (ws.ws_row) t.rows = ws.ws_row;
        if (ws.ws_xpixel && ws.ws_col) t.cell_w = ws.ws_xpixel / ws.ws_col;
        if (ws.ws_ypixel && ws.ws_row) t.cell_h = ws.ws_ypixel / ws.ws_row;
    }
    return t;
}

// HSV to RGB. h in degrees, s and v in 0..1 (used by --rainbow)
inline Rgb hsv_to_rgb(double h, double s, double v) {
    double c = v * s;
    double hp = std::fmod(std::fmod(h, 360.0) + 360.0, 360.0) / 60.0;
    double x = c * (1.0 - std::fabs(std::fmod(hp, 2.0) - 1.0));
    double r = 0, g = 0, b = 0;
    if (hp < 1) { r = c; g = x; }
    else if (hp < 2) { r = x; g = c; }
    else if (hp < 3) { g = c; b = x; }
    else if (hp < 4) { g = x; b = c; }
    else if (hp < 5) { r = x; b = c; }
    else { r = c; b = x; }
    double m = v - c;
    auto to8 = [](double f) { return static_cast<uint8_t>(std::lround(f * 255.0)); };
    return Rgb{to8(r + m), to8(g + m), to8(b + m)};
}

// Vertical position of the block: 0 is top, 50 is centered, 100 is bottom
inline int g_vpos = 50;

inline int top_row(int rows, int height) {
    return std::max(0, (rows - height) * g_vpos / 100);
}

}  // namespace util
