#pragma once
// Lyrics: lookup on lrclib.net, disk cache, .lrc saving, LRC parsing, version candidates.

#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <system_error>
#include <thread>
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

// Performs one GET request. Returns {HTTP code, body}, or nullopt on a transport error
inline std::optional<std::pair<long, std::string>> http_get(const std::string& url) {
    CURL* curl = curl_easy_init();
    if (!curl) return std::nullopt;

    std::string body;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &body);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "lyrics-term/2.0.0");

    CURLcode rc = curl_easy_perform(curl);
    long code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
    curl_easy_cleanup(curl);

    if (rc != CURLE_OK) return std::nullopt;
    return std::make_pair(code, body);
}

// URL-encodes a value for a query string
inline std::string escape(const std::string& value) {
    CURL* curl = curl_easy_init();
    if (!curl) return value;
    char* e = curl_easy_escape(curl, value.c_str(), static_cast<int>(value.size()));
    std::string out = e ? e : value;
    curl_free(e);
    curl_easy_cleanup(curl);
    return out;
}

// nullopt: network error (not cached). Empty string: no synced lyrics. Otherwise: syncedLyrics
inline std::optional<std::string> fetch_synced(const std::string& artist, const std::string& title) {
    std::string url = "https://lrclib.net/api/get?artist_name=" + escape(artist) + "&track_name=" + escape(title);
    auto res = http_get(url);
    if (!res) return std::nullopt;
    if (res->first == 404) return std::string();
    if (res->first != 200) return std::nullopt;

    try {
        auto j = nlohmann::json::parse(res->second);
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

// Finds a local .lrc for a track: "Artist - Title.lrc" in the folder first, then any .lrc in the
// folder tree whose file name contains both the artist and the title (case-insensitive).
// Returns an empty string if nothing matches
inline std::string find_local_lrc(const std::string& dir, const std::string& artist, const std::string& title) {
    namespace fs = std::filesystem;
    std::error_code ec;
    if (dir.empty() || (artist.empty() && title.empty()) || !fs::is_directory(dir, ec)) return {};

    fs::path exact = fs::path(dir) / (safe_name(artist + " - " + title) + ".lrc");
    if (fs::exists(exact, ec)) return exact.string();

    auto lower = [](std::string s) {
        for (auto& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        return s;
    };
    const std::string a = lower(artist), t = lower(title);

    fs::recursive_directory_iterator it(dir, fs::directory_options::skip_permission_denied, ec);
    fs::recursive_directory_iterator end;
    for (; !ec && it != end; it.increment(ec)) {
        const fs::path& p = it->path();
        if (p.extension() != ".lrc" || !it->is_regular_file(ec)) continue;
        std::string name = lower(p.filename().string());
        if ((a.empty() || name.find(a) != std::string::npos) && (t.empty() || name.find(t) != std::string::npos))
            return p.string();
    }
    return {};
}

// Lookup order for one track:
//   auto:   local folder, then cache, then lrclib
//   local:  local folder only
//   lrclib: cache and lrclib, folder skipped
inline std::optional<std::string> load_track(const std::string& artist, const std::string& title,
                                             const std::string& backend, const std::string& lyrics_dir) {
    if (backend == "auto" || backend == "local") {
        std::string path = find_local_lrc(lyrics_dir, artist, title);
        if (!path.empty()) {
            std::ifstream in(path, std::ios::binary);
            std::ostringstream ss;
            ss << in.rdbuf();
            if (!ss.str().empty()) return ss.str();
        }
        if (backend == "local") return std::string();
    }
    return load_synced(artist, title);
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

// Extracts distinct non-empty syncedLyrics values from a /api/search response body.
// Invalid JSON or a non-array body gives an empty result. No network access here, so it is testable
inline std::vector<std::string> parse_candidates_json(const std::string& body) {
    std::vector<std::string> out;
    try {
        auto j = nlohmann::json::parse(body);
        if (!j.is_array()) return out;
        for (const auto& item : j) {
            if (!item.contains("syncedLyrics") || !item["syncedLyrics"].is_string()) continue;
            std::string s = item["syncedLyrics"].get<std::string>();
            if (!s.empty() && std::find(out.begin(), out.end(), s) == out.end()) out.push_back(s);
        }
    } catch (...) {
        out.clear();
    }
    return out;
}

// All distinct synced versions lrclib knows for a track (used by the 'v' key).
// Retries transient failures (transport errors and HTTP 5xx) up to three times.
// nullopt: every attempt failed. Empty vector: the request worked and no synced version exists
inline std::optional<std::vector<std::string>> fetch_candidates(const std::string& artist,
                                                                const std::string& title) {
    std::string url = "https://lrclib.net/api/search?artist_name=" + escape(artist) +
                      "&track_name=" + escape(title);
    constexpr int kAttempts = 3;
    for (int attempt = 1; attempt <= kAttempts; ++attempt) {
        auto res = http_get(url);
        if (res && res->first == 200) return parse_candidates_json(res->second);
        if (res && res->first >= 400 && res->first < 500) return std::nullopt;  // no point retrying
        if (attempt < kAttempts) std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    return std::nullopt;
}

}  // namespace lyrics
