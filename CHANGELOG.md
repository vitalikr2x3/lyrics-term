## 3.1.14

- `--backend spicy` (private builds only): lyrics only from Spicy Lyrics. The public build rejects it.

## 3.1.13

- Private build option `-Dspicy=true`: Spicy Lyrics as a lyrics source (syllable sync turned into lines),
  needs `SPICY_LYRICS_SECRET_KEY`. Attribution is shown in the header. The default build does not contain this code.
- Player info carries the Spotify track id (MPRIS `xesam:url`), used by the Spicy source.

## 3.1.12

- Header symbol: `♪` by default instead of the 🎵 emoji. `--icon TEXT` sets another symbol,
  `--icon ""` hides it.

## 3.1.11

- `--glyph-size N` (6..16, default 8): rows per glyph in the 8x8 mode, so the TrueType glyphs can be drawn
  larger and more readable (for example `--glyph-size 10`).

## 3.1.10

- Block font 8x8: `--glyphs 8x8` draws letters rasterized from a TrueType font (default: Consolas or
  Courier New on Windows, DejaVu Sans Mono on Linux; or `--font PATH`). Each pixel is two cells wide.

## 3.1.9

- Block font size: `--scale 1..4` (default 1). Each pixel becomes a square of N x N cells;
  wrapping takes the scale into account. Works in config.kdl as `scale 2`.

## 3.1.8

- Block renderer clears the screen before each frame, so old lines disappear when the track ends
  or the lyrics are empty.

## 3.1.7

- Block renderer places every line by its absolute row (ESC[row;1H) instead of printing newlines,
  so frames no longer drift or duplicate in Windows consoles.

## 3.1.6

- Terminal: the program runs in the alternate screen buffer (like vim or less). The terminal's scrollback
  no longer shows old frames; the original screen comes back on exit. Headless mode is not affected.

## 3.1.5

- Block renderer redraws in place: the cursor goes home instead of clearing the screen with ESC[2J,
  so the terminal no longer scrolls on every update (visible on Windows Terminal).

## 3.1.4

- Build: ckdl is built through a Meson file in `subprojects/packagefiles/ckdl` (Meson wrap `patch_directory`).

## 3.1.3

- Build: ckdl is compiled from its C sources by Meson; CMake is no longer needed.

## 3.1.2

- Build: C++17 is requested as `c++17` (not `gnu++17`), so a clean Windows setup works with MSVC.

## 3.1.1

- Block font: added ( ) : ; " & / + * % = _ so titles and lyrics with these characters are drawn
  instead of showing "missing in font".

## 3.1.0

- Config file `config.kdl` (KDL): `~/.config/lyrics-term/config.kdl` on Linux, `%APPDATA%\lyrics-term\config.kdl` on Windows.
  Every command-line option works in the file; the command line overrides it. Use `--config PATH` or `--no-config`.
- KDL parsing uses ckdl (built from `subprojects/ckdl` with CMake).

## 3.0.0

- Two platforms from one codebase: Linux (MPRIS over D-Bus) and Windows (System Media Transport Controls).
  macOS is not supported.
- Platform code is isolated in `platform.hpp` and the `platform_*.cpp` / `player_*.cpp` files; `main.cpp` no longer
  includes termios, D-Bus or unistd headers.
- Windows terminal: ANSI output through the Console API, keys through `_kbhit`/`_getch`.
- Cache and home folders follow each system: `~/.cache` (Linux), `%LOCALAPPDATA%` (Windows).
- The kitty renderer and `--from-kitty` are available on Linux only; on Windows the block renderer is used.
- Status: Linux is the 2.1.0 code in the new layout. Windows is not compiled or tested yet.

# Changelog

## 2.1.0

- Local lyrics: lyrics are looked up first in a folder of `.lrc` files (`--lyrics-dir`, default `~/Lyrics`),
  matching the Caelestia shell's naming (`Artist - Title.lrc`), then in the cache and lrclib.
- `--backend auto|local|lrclib` chooses the source: `auto` (default), `local` (folder only), `lrclib` (cache and lrclib only).
- Unit test for the local folder lookup.
- `--headless` runs in the background: the terminal is no longer switched to raw mode in headless mode.

## 2.0.1

- New `--headless` mode: nothing is drawn, the current line is only written to the `--export` file.
  Used by the Quickshell lyrics shell (`integrations/quickshell/lyrics-shell`).
- The `--export` file is cleared when there is no player, while loading, and when no lyrics are found.

## 2.0.0

First stable release of the 2.0 line. Everything from the alphas and betas is included.

### Highlights
- Background lookups: the screen never freezes while lyrics or other versions load.
- `v` switches between other synced versions of a track, with retries and header hints.
- `--fade` fades new lines in (kitty), without flicker.
- Pasting of `--export` output into status bars: the current line goes to a file atomically.
- Works with Spotify, Audacious and mpv (with `mpv-mpris`).
- Built with Meson; CI builds and tests on Arch Linux; `flake.nix` builds with Meson and runs the tests.
- Man page and a Quickshell widget skeleton.

### Known limitations
- The Quickshell widget is a skeleton.
- `v` does not retry on client errors (4xx).

## 2.0.0-beta2

- Fixed flicker with `--fade` in kitty: the text no longer disappears during a fade. The screen is erased line by line
  instead of a full clear, and the new image is placed before the previous one is deleted.

## 2.0.0-beta1

- Feature freeze. Until 2.0.0, only bug fixes are accepted.
- No functional changes since 2.0.0-alpha4; the README status line and this changelog are updated.
- Known limitations: the Quickshell widget is a skeleton; `v` does not retry when lrclib returns a client error.

## 2.0.0-alpha4

- README: supported players (Spotify, Audacious, mpv) with setup notes, Meson install instructions,
  and the alpha status.
- PKGBUILD uses Meson (`arch-meson`, `meson compile`, `meson test`, `meson install`) instead of CMake.
- Tested on Spotify, Audacious and mpv (with `mpv-mpris`).

## 2.0.0-alpha3

- Build system switched from CMake to Meson. Configure with `meson setup build -Dtests=true`,
  build with `meson compile -C build`, test with `meson test -C build`, install with `meson install -C build`.
- CI workflow uses Meson and runs the same commands.
- The man page is installed through `install_man`.

## 2.0.0-alpha2

- `v` lookups retry transient network errors (up to three attempts, 500 ms apart). Client errors are not retried.
- The header shows a hint when versions are unavailable ("versions unavailable") or when the track has no other
  synced version ("no other versions"), and "looking for versions..." while the request runs.
- JSON parsing of lrclib search results moved into `parse_candidates_json`, which is covered by unit tests.
- Unit tests for candidate parsing use made-up input; no network access is needed to run them.

## 2.0.0-alpha1

- Background lookup of other synced versions for the `v` key.
- Unit tests (`-DLYRICS_TERM_TESTS=ON`) and a GitHub Actions workflow for Arch Linux.
- Man page and a Quickshell widget skeleton.
- Known limitations: not tested on a live player yet; the `v` lookup is still a single request
  without retries; the Quickshell widget is a skeleton.

## 1.1.0

- `--fade`: a new line fades in over four frames (kitty renderer only).
- `v` key: switch to the next synced version of the current track, if lrclib has more than one.
  The version counter is shown in the header.
- Renderer opacity is now a float: dimming while paused and fading use the same code path.

## 1.0.0

- First release: MPRIS player support, lrclib lookups with disk cache, block and kitty renderers,
  line and word modes, sync shift, `--export`, `--save-lrc`.
