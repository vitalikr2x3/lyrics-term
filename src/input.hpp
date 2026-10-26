#pragma once
// Non-blocking key input. The terminal is switched to raw mode without echo.
// Ctrl+C still works because ISIG stays enabled.

#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

namespace input {

inline termios g_orig{};
inline bool g_active = false;

inline void enable_raw() {
    if (!isatty(STDIN_FILENO)) return;
    if (tcgetattr(STDIN_FILENO, &g_orig) != 0) return;
    termios raw = g_orig;
    raw.c_lflag &= ~(ICANON | ECHO);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) == 0) g_active = true;
}

// Called both on exit and from the signal handler (tcsetattr is async-signal-safe)
inline void restore() {
    if (g_active) {
        tcsetattr(STDIN_FILENO, TCSANOW, &g_orig);
        g_active = false;
    }
}

// Returns a pressed key, or 0 if nothing is waiting
inline char poll_key() {
    if (!g_active) return 0;
    fd_set set;
    FD_ZERO(&set);
    FD_SET(STDIN_FILENO, &set);
    timeval tv{0, 0};
    if (select(STDIN_FILENO + 1, &set, nullptr, nullptr, &tv) > 0) {
        char c = 0;
        if (read(STDIN_FILENO, &c, 1) == 1) return c;
    }
    return 0;
}

}  // namespace input
