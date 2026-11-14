// macOS player source: Spotify and Music through AppleScript (macOS has no MPRIS).
// The first call asks for permission to control each app: System Settings, Privacy & Security, Automation.
#include "platform.hpp"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

namespace {

const char* const kApps[] = {"Spotify", "Music"};

std::string lower(std::string s) {
    for (auto& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

// Runs osascript with the given script lines and returns its output without the trailing newline
std::string run_script(const std::vector<std::string>& lines) {
    std::string cmd = "osascript";
    for (const auto& l : lines) cmd += " -e '" + l + "'";
    cmd += " 2>/dev/null";
    std::string out;
    FILE* p = popen(cmd.c_str(), "r");
    if (!p) return out;
    char buf[1024];
    while (std::fgets(buf, sizeof buf, p)) out += buf;
    pclose(p);
    while (!out.empty() && (out.back() == '\n' || out.back() == '\r')) out.pop_back();
    return out;
}

// The current track of one app. Empty if the app is not running or has no track
std::optional<PlayerInfo> query_app(const std::string& app) {
    const std::string quoted = "\"" + std::string(app) + "\"";
    // "is running" does not launch the app, unlike "tell application"
    std::string out = run_script({
        "if application " + quoted + " is running then",
        "tell application " + quoted,
        "if player state is stopped then return \"\"",
        "return (artist of current track) & tab & (name of current track) & tab & (player state as text) & tab & (player position as text)",
        "end tell",
        "end if",
    });
    if (out.empty()) return std::nullopt;

    std::vector<std::string> f;
    std::string cur;
    for (char c : out) {
        if (c == '\t') {
            f.push_back(cur);
            cur.clear();
        } else {
            cur.push_back(c);
        }
    }
    f.push_back(cur);
    if (f.size() < 4 || f[1].empty()) return std::nullopt;

    PlayerInfo p;
    p.name = lower(app);
    p.artist = f[0];
    p.title = f[1];
    const std::string state = lower(f[2]);
    p.status = state == "playing" ? "Playing" : state == "paused" ? "Paused" : "Stopped";
    std::string pos = f[3];
    for (auto& c : pos) {
        if (c == ',') c = '.';  // some locales print a decimal comma
    }
    p.position = std::strtod(pos.c_str(), nullptr);
    return p;
}

}  // namespace

std::optional<PlayerInfo> best_player(const std::string& wanted) {
    std::optional<PlayerInfo> chosen;
    for (const char* app : kApps) {
        if (!wanted.empty() && lower(app).find(wanted) == std::string::npos) continue;
        auto p = query_app(app);
        if (!p) continue;
        if (!chosen || (p->status == "Playing" && chosen->status != "Playing")) chosen = p;
    }
    return chosen;
}

std::vector<std::string> list_players() {
    std::vector<std::string> out;
    for (const char* app : kApps) {
        const std::string quoted = "\"" + std::string(app) + "\"";
        if (run_script({"application " + quoted + " is running"}) == "true") out.push_back(lower(app));
    }
    return out;
}
