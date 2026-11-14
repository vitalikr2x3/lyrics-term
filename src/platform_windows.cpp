// Terminal for Windows: Console API. ANSI sequences are enabled on the output,
// keys are read with _kbhit/_getch.
#include "platform.hpp"

#include <conio.h>
#include <windows.h>

namespace {
bool g_active = false;
}

TermSize term_size() {
    TermSize t;
    CONSOLE_SCREEN_BUFFER_INFO info{};
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info)) {
        t.cols = info.srWindow.Right - info.srWindow.Left + 1;
        t.rows = info.srWindow.Bottom - info.srWindow.Top + 1;
    }
    // The console does not report the cell size in pixels; only the kitty renderer needs it
    return t;
}

void enable_terminal() {
    // Virtual terminal processing: ANSI escape sequences work in Windows 10 (1511) and newer
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (GetConsoleMode(out, &mode)) SetConsoleMode(out, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    g_active = true;
}

void restore_terminal() {
    g_active = false;
}

int poll_key() {
    if (!g_active || !_kbhit()) return 0;
    return _getch();
}
