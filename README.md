# lyrics-term

Terminal player for synced song lyrics. It follows the music player that is playing right now and shows the current line of the song in the terminal, in large block letters or as a true-font image in kitty.

Lyrics come from a folder of `.lrc` files, the local cache, or [lrclib.net](https://lrclib.net), in that order.

> **Branches:** `main` is lyrics-term 3.x (C++, Linux and Windows). The 2.x line (C++ with CMake, Linux only) lives in the `v2-line` branch.

**Status:** 3.0.2-alpha1. Linux is tested. Windows is not tested yet. macOS is not supported.

## Features

- Works with any MPRIS player on Linux (Spotify, Audacious, mpv, VLC, browsers) and with the Windows media session (Spotify and other apps that show in the Windows media flyout).
- Block font with Latin, Cyrillic and Ukrainian letters, in any terminal.
- Kitty renderer: the line is drawn with a true font as an image, with an optional glow (Linux).
- Line and word modes, previous and next lines, rainbow colors, dimming while paused, fade-in of new lines (kitty).
- Lyrics are loaded in the background, so the screen does not freeze while a lookup runs.
- `v` switches between other synced versions of a track, when lrclib has more than one.
- Manual sync shift with `+` and `-`.
- `--export` writes the current line to a file atomically, for status bars and shells.
- `--headless` draws nothing and only writes the export file, for running in the background.

## Platforms

| Platform | Player source | Terminal | Kitty renderer |
|----------|---------------|----------|----------------|
| Linux | MPRIS over D-Bus | termios | yes |
| Windows | System Media Transport Controls (SMTC) | Console API with ANSI output | no (block renderer only) |

## Installation

### Arch Linux

Dependencies:

```bash
sudo pacman -S --needed base-devel meson ninja pkgconf dbus curl freetype2 nlohmann-json noto-fonts ttf-dejavu
```

Build and install:

```bash
git clone https://github.com/vitalikr2x3/lyrics-term.git
cd lyrics-term
meson setup build --buildtype=release
meson compile -C build
sudo meson install -C build
```

The kitty renderer needs kitty itself. To read colors from the kitty palette (`--from-kitty`), add `allow_remote_control yes` to `kitty.conf`.

### Windows

Requirements: Visual Studio 2022 with the "Desktop development with C++" workload and the Windows 10 SDK, Meson, Ninja, and libcurl, freetype and nlohmann-json (for example through vcpkg).

From a Developer PowerShell:

```powershell
git clone https://github.com/vitalikr2x3/lyrics-term.git
cd lyrics-term
meson setup build --buildtype=release
meson compile -C build
```

The result is `build\lyrics-term.exe`. Start the program while Spotify (or another app) is playing music.

## Usage

```bash
lyrics-term --list-players                            # show players the program can see
lyrics-term                                           # follow whichever player is playing
lyrics-term --player spotify -c "#00ffcc"             # one player, custom color
lyrics-term --clean --rainbow --next -c "#ff0077"     # title and lyrics only
lyrics-term --renderer kitty --style soft --from-kitty color2 --fade
lyrics-term --mode word --renderer block              # one word at a time, block font
lyrics-term --prev --next --vpos 20 --dim-paused      # neighbouring lines, text near the top
lyrics-term --export ~/.cache/lyrics-term/current.txt # current line into a file
lyrics-term --headless --export ~/.cache/lyrics-term/current.txt &
```

Run `lyrics-term --help` for the full list of options.

### Controls

| Key | Action |
|-----|--------|
| `+` / `-` | shift the sync by 0.1 s |
| `0` | reset the shift |
| `v` | switch to the next synced version of the track |
| `q` | quit |

## Lyrics sources

`--backend` chooses where lyrics come from:

- `auto` (default): the folder of `.lrc` files, then the cache, then lrclib.
- `local`: the folder only.
- `lrclib`: the cache and lrclib only.

The folder is `~/Lyrics` by default, or the path given with `--lyrics-dir`. A file is found when its name is `Artist - Title.lrc`, or when its name contains both the artist and the title (case-insensitive). The cache is kept in `~/.cache/lyrics-term` (Linux) or `%LOCALAPPDATA%\lyrics-term` (Windows).

`--save-lrc DIR` saves every `.lrc` file that lrclib returns into `DIR`.

## Colors

`-c` and `--from-kitty` accept a palette slot name, a named color, or a HEX value:

- Named: `cyan`, `green`, `red`, `yellow`, `blue`, `magenta`, `white`.
- HEX: `#00ffcc` or `00ffcc`.
- kitty palette (Linux, kitty only): `foreground`, `color0` to `color15`.

## Status bar integration

`--export` writes the current line to a file. The file is replaced atomically, so a reader never sees a half-written line:

```bash
lyrics-term --headless --export ~/.cache/lyrics-term/current.txt &
```

A Waybar or Quickshell module can read this file on a timer. A Quickshell skeleton is in `integrations/quickshell/`.

## Notes

- On Linux, sync depends on the player reporting its `Position` over MPRIS.
- On Windows, the position comes from the media session and is extrapolated between its updates, so sync can drift slightly.
- A track is found only when the artist and the title in the player's metadata are correct. Files named by the app itself (for example, a random string) will not match.
- A failed network request is not cached. A track without synced lyrics is cached as empty; to retry it, delete its file in the cache folder.
- The header counts emoji as one column, so the header may be shifted by one column.

## Tests

```bash
meson setup build -Dtests=true
meson test -C build --print-errorlogs
```

## License

MIT. See [LICENSE](LICENSE).
