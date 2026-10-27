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
