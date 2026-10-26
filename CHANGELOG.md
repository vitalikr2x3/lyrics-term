# Changelog

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
