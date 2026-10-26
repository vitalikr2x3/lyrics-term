# Roadmap

## Done in 2.0.0-alpha1

- Background lookup of other synced versions (`v` key no longer blocks the loop).
- Unit tests for LRC parsing, word timing and file names (`-DLYRICS_TERM_TESTS=ON`).
- GitHub Actions build and test in an Arch Linux container.
- Man page (`man/lyrics-term.1`).
- Quickshell widget skeleton that reads the `--export` file (`integrations/quickshell/`).

## Planned

- Equalizer under the line, fed by `cava` (decide between a shared file and reading `cava` output).
- Accent color from the current track's cover art, passed to `-c`.
- Spicetify theme that matches the colors used by lyrics-term.
- Blink or color cycling for the block renderer.
- Shell completions for fish and bash.
- GPG-signed packages in the personal pacman repository.
- Windows port: a second player backend (SMTC) behind the same interface as MPRIS.
