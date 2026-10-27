# lyrics-term

Terminal player for synced song lyrics. It works with any MPRIS-compatible player
and draws the current line either with a built-in block font or as an image in kitty.

Lyrics come from [lrclib.net](https://lrclib.net) and are cached on disk.

**Status: 2.0.0, stable.** Tested on Spotify, Audacious and mpv. Report problems on the issue tracker.

## Features

- Works with any MPRIS player; pick one with `--player`
- Background lookups: the screen does not freeze while lyrics load
- Other synced versions of a track, switched with `v` (retries on network errors)
- Disk cache in `~/.cache/lyrics-term/`
- Block font with Latin, Cyrillic and Ukrainian letters, or a true-font image in kitty
- Line and word modes, previous and next lines, rainbow colors, dimming while paused
- Fade-in of new lines in kitty (`--fade`)
- Manual sync shift with `+` and `-`
- Export of the current line to a file for status bars
- Saving of found `.lrc` files

## Supported players

| Player | What to do |
|--------|-----------|
| Spotify | Nothing: it exposes MPRIS itself |
| Audacious | Enable the **MPRIS2** plugin: Ctrl+P → Plugins → MPRIS2, then restart |
| mpv | Install `mpv-mpris` (`sudo pacman -S mpv-mpris`). The plugin loads from `/etc/mpv/scripts`, `mpv.conf` needs no changes |

Check that the player is visible: `lyrics-term --list-players`.

The player must report a title in its metadata. Tracks without artist and title tags
will not match lrclib.

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

The repository holds the stable release only. Alpha builds are available from source.

### Arch Linux: build from source

```bash
git clone https://github.com/vitalikr2x3/lyrics-term.git
cd lyrics-term
git checkout v2.0.0-alpha4
makepkg -si
```

Dependencies on Arch Linux:

```bash
sudo pacman -S --needed base-devel meson ninja pkgconf dbus curl freetype2 nlohmann-json noto-fonts ttf-dejavu
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

### Other distributions: Meson

Requirements: Meson, Ninja, pkg-config, libdbus, libcurl, FreeType, nlohmann-json and a C++17 compiler.

```bash
git clone https://github.com/vitalikr2x3/lyrics-term.git
cd lyrics-term
meson setup build --buildtype=release
meson compile -C build
sudo meson install -C build
```

To run the unit tests: `meson setup build -Dtests=true` and then `meson test -C build`.

## Usage

```bash
lyrics-term --list-players                            # show MPRIS players
lyrics-term                                           # follow whichever player is playing
lyrics-term --player audacious -c "#00ffcc"           # one player, custom color
lyrics-term --player mpv --no-header                  # mpv without the header
lyrics-term --clean --rainbow --next -c "#ff0077"     # title and lyrics only
lyrics-term --renderer kitty --style soft --from-kitty color2 --fade
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
| `v` | switch to the next synced version of the track, if lrclib has one |
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

A Waybar or Quickshell module can read this file on a timer. A Quickshell skeleton is in
`integrations/quickshell/`.

## Notes

- Sync depends on the player reporting its `Position` over MPRIS.
- Lookups need the correct artist and title in the track metadata. Files named by the
  app itself (for example, a random string) will not match.
- A failed network request is not cached. A track without synced lyrics is cached as
  empty; to retry it, delete its file in `~/.cache/lyrics-term/`.
- Emoji in the header are counted as one column, so the header may be shifted by one column.

## License

MIT. See [LICENSE](LICENSE).
