# lyrics-shell

A small Quickshell configuration that shows the current synced lyric line at the bottom of
the screen. It starts `lyrics-term` in headless mode itself, so you do not need an
`exec-once` line in Hyprland.

## Requirements

- Quickshell (`quickshell` in the Arch repositories)
- `lyrics-term` 2.0.1 or newer in `PATH`
- A player with MPRIS (Spotify, Audacious, mpv with `mpv-mpris`)

## Install

```bash
cd lyrics-shell
./install.sh
```

The script copies `shell.qml` into `~/.config/quickshell/lyrics/`.

## Run

```bash
qs -c lyrics
```

To start it with Hyprland, add this to `~/.config/hypr/hyprland.conf`:

```ini
exec-once = qs -c lyrics
```

## Settings

Open `~/.config/quickshell/lyrics/shell.qml` and change the values at the top:

| Setting | Meaning |
|---------|---------|
| `player` | `""` for any player, or `"spotify"`, `"audacious"`, `"mpv"` |
| `accent` | text color |
| `fontFamily`, `fontSize` | font |
| `barHeight` | height of the bar in pixels |
| `fadeMs` | fade time between lines |
| `backgroundOpacity` | opacity of the dark background, 0 means transparent |

The bar is hidden when there is no line to show, for example when nothing plays.

## Notes

- The shell reads `~/.cache/lyrics-term/current.txt`, which `lyrics-term --headless` writes.
- If you already run a full shell such as Caelestia, you can keep both: each `qs -c` name is a separate configuration.
- This is version 2.0.1 of the shell. The Quickshell API calls (`FileView`, `Process`, `PanelWindow`) should be checked against your installed Quickshell version if something does not start.
