#pragma once
#include <string>

// Home folder, "" if unknown: $HOME on Linux and macOS, %USERPROFILE% on Windows
std::string home_dir();
// Per-user cache root: $XDG_CACHE_HOME or ~/.cache on Linux, ~/Library/Caches on macOS, %LOCALAPPDATA% on Windows
std::string cache_root();
// True if the file can be opened for reading
bool file_readable(const std::string& path);

// Folder for config.kdl: %APPDATA%\lyrics-term on Windows, $XDG_CONFIG_HOME/lyrics-term or ~/.config/lyrics-term on Linux
std::string config_dir();
