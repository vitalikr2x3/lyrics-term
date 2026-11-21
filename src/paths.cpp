#include "paths.hpp"

#include <cstdlib>
#include <fstream>

std::string home_dir() {
#ifdef _WIN32
    const char* h = std::getenv("USERPROFILE");
#else
    const char* h = std::getenv("HOME");
#endif
    return h ? h : "";
}

std::string cache_root() {
#if defined(_WIN32)
    if (const char* local = std::getenv("LOCALAPPDATA"); local && *local) return local;
    return home_dir() + "/AppData/Local";
#elif defined(__APPLE__)
    return home_dir() + "/Library/Caches";
#else
    if (const char* xdg = std::getenv("XDG_CACHE_HOME"); xdg && *xdg) return xdg;
    return home_dir() + "/.cache";
#endif
}

bool file_readable(const std::string& path) {
    std::ifstream f(path);
    return f.good();
}

std::string config_dir() {
#ifdef _WIN32
    if (const char* appdata = std::getenv("APPDATA"); appdata && *appdata) {
        return std::string(appdata) + "/lyrics-term";
    }
    return home_dir() + "/AppData/Roaming/lyrics-term";
#else
    if (const char* xdg = std::getenv("XDG_CONFIG_HOME"); xdg && *xdg) {
        return std::string(xdg) + "/lyrics-term";
    }
    return home_dir() + "/.config/lyrics-term";
#endif
}
