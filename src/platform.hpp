#pragma once
// Platform layer: the only code that differs between Linux, macOS and Windows.
// Implementations: platform_posix.cpp (terminal, Linux and macOS), platform_windows.cpp (terminal, Windows),
// player_linux.cpp (MPRIS), player_macos.cpp (AppleScript), player_windows.cpp (SMTC).

#include <optional>
#include <string>
#include <vector>

struct PlayerInfo {
    std::string name;     // short name: spotify, mpv, audacious, music, Spotify.exe
    std::string artist;
    std::string title;
    std::string status;   // Playing / Paused / Stopped
    double position = 0;  // seconds
};

// Player: `wanted` is a lowercase substring of the name ("" = any player).
// A playing player wins over a paused one. Throws std::runtime_error if the player service is unavailable
std::optional<PlayerInfo> best_player(const std::string& wanted);

// Short names of the running players
std::vector<std::string> list_players();

// Terminal
struct TermSize {
    int cols = 80, rows = 24;
    int cell_w = 10, cell_h = 20;  // pixels; the defaults are used when the terminal does not report them
};
TermSize term_size();
void enable_terminal();   // raw key input without echo; a no-op when stdin is not a terminal
void restore_terminal();  // undo enable_terminal(); safe to call from a signal handler on POSIX
int poll_key();           // a pressed key, or 0 if nothing is waiting; never blocks
