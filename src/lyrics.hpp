#pragma once
// Lyrics: lookup on lrclib.net, disk cache, .lrc saving, LRC parsing.

#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <system_error>
#include <vector>

struct LyricLine {
    double time;  // seconds from the start of the track
    std::string text;
};

namespace lyrics {

inline size_t write_cb(char* data, size_t size, size_t n, void* out) {
    static_cast<std::string*>(out)->append(data, size * n);
    return size * n;
}

inline std::string trim(const std::string& s) {
    const char* ws = " \t\r\n";
    auto b = s.find_first_not_of(ws);
    if (b == std::string::npos) return "";
    auto e = s.find_last_not_of(ws);
    return s.substr(b, e - b + 1);
}

// nullopt: network error (not cached). Empty string: no synced lyrics. Otherwise: syncedLyrics
inline std::optional<std::string> fetch_synced(const std::string& artist, const std::string& title) {
    CURL* curl = curl_easy_init();
    if (!curl) return std::nullopt;

    char* a = curl_easy_escape(curl, artist.c_str(), static_cast<int>(artist.size()));
    char* t = curl_easy_escape(curl, title.c_str(), static_cast<int>(title.size()));
    std::string url = std::string("https://lrclib.net/api/get?artist_name=") + a + "&track_name=" + t;
    curl_free(a);
    curl_free(t);

    std::string body;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &body);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "lyrics-term/1.0");

    CURLcode rc = curl_easy_perform(curl);
    long code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    curl_easy_cleanup(curl);

    if (rc != CURLE_OK) return std::nullopt;
    if (code == 404) return std::string();
    if (code != 200) return std::nullopt;

    try {
        auto j = nlohmann::json::parse(body);
        if (j.contains("syncedLyrics") && j["syncedLyrics"].is_string())
            return j["syncedLyrics"].get<std::string>();
    } catch (...) {
    }
    return std::string();
}

// ~/.cache/lyrics-term (or $XDG_CACHE_HOME/lyrics-term)
inline std::filesystem::path cache_dir() {
    namespace fs = std::filesystem;
    fs::path base;
    if (const char* xdg = std::getenv("XDG_CACHE_HOME"); xdg && *xdg) base = xdg;
    else if (const char* home = std::getenv("HOME")) base = fs::path(home) / ".cache";
    else base = ".cache";
    fs::path dir = base / "lyrics-term";
    std::error_code ec;
    fs::create_directories(dir, ec);
    return dir;
}

// Replaces characters that are not allowed in file names
inline std::string safe_name(const std::string& s) {
    std::string out;
    for (char c : s) {
        unsigned char u = static_cast<unsigned char>(c);
        bool bad = u < 0x20 || c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' ||
                   c == '"' || c == '<' || c == '>' || c == '|';
        out += bad ? '_' : c;
    }
    if (out.size() > 180) out.resize(180);
    return out;
}

// Disk cache first, then the network. Network errors are not cached
inline std::optional<std::string> load_synced(const std::string& artist, const std::string& title) {
    namespace fs = std::filesystem;
    fs::path file = cache_dir() / (safe_name(artist + " - " + title) + ".lrc");
    if (fs::exists(file)) {
        std::ifstream in(file, std::ios::binary);
        std::ostringstream ss;
        ss << in.rdbuf();
        return ss.str();
    }
    auto fetched = fetch_synced(artist, title);
    if (fetched) {
        std::ofstream out(file, std::ios::binary | std::ios::trunc);
        out << *fetched;
    }
    return fetched;
}

// Saves a found synced text into a directory (used by --save-lrc)
inline void save_lrc(const std::string& dir, const std::string& artist, const std::string& title,
                     const std::string& text) {
    namespace fs = std::filesystem;
    std::error_code ec;
    fs::create_directories(dir, ec);
    std::ofstream out(fs::path(dir) / (safe_name(artist + " - " + title) + ".lrc"),
                      std::ios::binary | std::ios::trunc);
    out << text;
}

// Parses LRC. Lines with several timestamps are supported; the result is sorted by time
inline std::vector<LyricLine> parse_lrc(const std::string& text) {
    std::vector<LyricLine> out;
    std::istringstream in(text);
    std::string raw;
    while (std::getline(in, raw)) {
        std::vector<double> stamps;
        size_t pos = 0;
        while (pos < raw.size() && raw[pos] == '[') {
            size_t close = raw.find(']', pos);
            if (close == std::string::npos) break;
            std::string tag = raw.substr(pos + 1, close - pos - 1);
            size_t colon = tag.find(':');
            if (colon != std::string::npos && colon > 0 && std::isdigit(static_cast<unsigned char>(tag[0]))) {
                try {
                    double m = std::stod(tag.substr(0, colon));
                    double s = std::stod(tag.substr(colon + 1));
                    stamps.push_back(m * 60.0 + s);
                } catch (...) {
                }
            }
            pos = close + 1;
        }
        if (stamps.empty()) continue;
        std::string content = trim(raw.substr(pos));
        for (double t : stamps) out.push_back({t, content});
    }
    std::stable_sort(out.begin(), out.end(),
                     [](const LyricLine& a, const LyricLine& b) { return a.time < b.time; });
    return out;
}

// Line mode returns the lines unchanged. Word mode gives each word its own timestamp
// inside the time span of its line
inline std::vector<LyricLine> build_units(const std::vector<LyricLine>& lines, bool word_mode) {
    if (!word_mode) return lines;

    std::vector<LyricLine> out;
    for (size_t i = 0; i < lines.size(); ++i) {
        std::istringstream ws(lines[i].text);
        std::vector<std::string> words;
        std::string w;
        while (ws >> w) words.push_back(w);
        if (words.empty()) {
            out.push_back({lines[i].time, ""});
            continue;
        }
        double start = lines[i].time;
        double end = i + 1 < lines.size() ? lines[i + 1].time : start + 3.0;
        if (end <= start) end = start + 0.1 * static_cast<double>(words.size());
        double step = (end - start) / static_cast<double>(words.size());
        for (size_t j = 0; j < words.size(); ++j)
            out.push_back({start + static_cast<double>(j) * step, words[j]});
    }
    std::stable_sort(out.begin(), out.end(),
                     [](const LyricLine& a, const LyricLine& b) { return a.time < b.time; });
    return out;
}

}  // namespace lyrics
