// Terminal for Linux and macOS: termios, select and the window size ioctl.
#include "platform.hpp"

#include <sys/ioctl.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

namespace {
termios g_orig{};
bool g_active = false;
}

TermSize term_size() {
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

void enable_terminal() {
    if (!isatty(STDIN_FILENO)) return;
    if (tcgetattr(STDIN_FILENO, &g_orig) != 0) return;
    termios raw = g_orig;
    raw.c_lflag &= ~(ICANON | ECHO);  // ISIG stays on, so Ctrl+C still works
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) == 0) g_active = true;
}

void restore_terminal() {
    if (g_active) {
        tcsetattr(STDIN_FILENO, TCSANOW, &g_orig);
        g_active = false;
    }
}

int poll_key() {
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
