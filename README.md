# lyrics-term

Terminal player for synced song lyrics. It works with any MPRIS-compatible player
(Spotify, Audacious, mpv, VLC, browsers) and draws the current line either with a
built-in block font or as an image in kitty.

Lyrics come from [lrclib.net](https://lrclib.net) and are cached on disk.

## Features

- Works with any MPRIS player; pick one with `--player`
- Background lookups, so the screen never freezes while lyrics load
- Disk cache in `~/.cache/lyrics-term/`
- Block font with Latin, Cyrillic and Ukrainian letters, or a true-font image in kitty
- Line and word modes, previous and next lines, rainbow colors, dimming while paused
- Manual sync shift with `+` and `-`
- Export of the current line to a file for status bars
- Saving of found `.lrc` files

## Installation

### Arch Linux: pacman repository

Add this to the end of `/etc/pacman.conf`, below the official repositories:

```ini
[vitalikr2x3]
SigLevel = Optional TrustAll
Server = https://vitalikr2x3.github.io/vitalik-repo/$arch
```

Then install:

```bash
sudo pacman -Sy lyrics-term
```

### Arch Linux: build from source

```bash
git clone https://github.com/vitalikr2x3/lyrics-term.git
cd lyrics-term
makepkg -si
```

Dependencies on Arch Linux:

```bash
sudo pacman -S --needed base-devel cmake pkgconf dbus curl freetype2 nlohmann-json noto-fonts ttf-dejavu
```

### NixOS

Run without installing:

```bash
nix run github:vitalikr2x3/lyrics-term -- --list-players
```

Install into your profile:

```bash
nix profile install github:vitalikr2x3/lyrics-term
```

Or add it to a system flake:

```nix
inputs.lyrics-term.url = "github:vitalikr2x3/lyrics-term";
```

and use `inputs.lyrics-term.packages.${pkgs.system}.default` in `environment.systemPackages`.

On NixOS there are no fonts in `/usr/share/fonts`, so for the kitty renderer pass the font path with `--font`, for example `--font ${pkgs.noto-fonts}/share/fonts/noto/NotoSans-Bold.ttf`.

### Other distributions: CMake

Requirements: CMake, pkg-config, libdbus, libcurl, FreeType, nlohmann-json and a C++17 compiler.

```bash
git clone https://github.com/vitalikr2x3/lyrics-term.git
cd lyrics-term
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
sudo cmake --install build
```

## Usage

```bash
lyrics-term --list-players                            # show MPRIS players
lyrics-term                                           # follow whichever player is playing
lyrics-term --player audacious -c "#00ffcc"           # one player, custom color
lyrics-term --clean --rainbow --next -c "#ff0077"     # title and lyrics only
lyrics-term --renderer kitty --style soft --from-kitty color2
lyrics-term --renderer kitty --style mono --font-size 64 --mode word
lyrics-term --prev --next --vpos 20 --dim-paused      # neighbouring lines, text near the top
lyrics-term --export ~/.cache/lyrics-term/current.txt # current line into a file
lyrics-term --save-lrc ~/Music/lrc                    # keep found .lrc files
```

Run `lyrics-term --help` for the full option list.

### Controls

| Key | Action |
|-----|--------|
| `+` / `-` | shift the sync by 0.1 s |
| `0` | reset the shift |
| `q` | quit |

## Colors

`-c` and `--from-kitty` accept a palette slot name, a named color, or a HEX value:

- Named: `cyan`, `green`, `red`, `yellow`, `blue`, `magenta`, `white`
- HEX: `#00ffcc` or `00ffcc`
- kitty palette (kitty renderer only): `foreground`, `color0` to `color15`

Reading the kitty palette needs `allow_remote_control yes` in `kitty.conf`.

## Status bar integration

`--export` writes the current line to a file. The file is replaced atomically, so a
status bar never reads a half-written line:

```bash
lyrics-term --export ~/.cache/lyrics-term/current.txt --no-header
```

A Waybar or Quickshell module can read this file on a timer.

## Notes

- Sync depends on the player reporting its `Position` over MPRIS.
- Lookups need the correct artist and title in the track metadata. Files named by
  the app itself (for example, a random string) will not match.
- A failed network request is not cached. A track without synced lyrics is cached
  as empty; to retry it, delete its file in `~/.cache/lyrics-term/`.
- Emoji in the header are counted as one column, so the header may be shifted by one column.

## License

MIT. See [LICENSE](LICENSE).
