// lyrics-term: synced song lyrics for any MPRIS-compatible music player.

#include <curl/curl.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <future>
#include <initializer_list>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "block.hpp"
#include "input.hpp"
#include "kitty.hpp"
#include "lyrics.hpp"
#include "mpris.hpp"
#include "util.hpp"

namespace {

struct Options {
    std::string color = "cyan";
    std::string from_kitty;
    std::string player;
    bool list_players = false;
    bool no_header = false;
    bool no_artist = false;
    bool no_source = false;
    bool no_missing = false;
    bool left = false;
    bool no_bold = false;
    bool word_mode = false;
    bool kitty = false;
    bool rainbow = false;
    bool prev_line = false;
    bool next_line = false;
    bool dim_paused = false;
    bool fade = false;
    bool headless = false;
    std::string backend = "auto";  // auto | local | lrclib
    std::string lyrics_dir;         // folder with .lrc files; default: ~/Lyrics  // draw nothing; only write the --export file (for Quickshell and bars)
    int vpos = 50;
    std::string export_path;
    std::string save_lrc_dir;
    std::string style = "soft";
    std::string font;
    int font_size = 56;
    int spacing = -1;  // -1: use the style value
    int glow = -1;     // -1: use the style value, 0 off, 1 on
};

void print_help() {
    std::cout << R"(lyrics-term - synced song lyrics for MPRIS music players

Colors and layout:
  -c, --color COLOR          name (cyan, green...) or HEX (#00ffcc)
      --from-kitty COLOR     color from the kitty palette (color2, foreground) or HEX
      --align left|center    horizontal alignment (default: center)
      --vpos N               vertical position 0-100 (default: 50)
      --rainbow              a different hue for every line
      --dim-paused           dim the text while paused
      --fade                 fade each new line in (kitty only)
      --headless             draw nothing; only write the --export file (for Quickshell and bars)
      --backend auto|local|lrclib
                             where lyrics come from: local folder, then cache and lrclib (auto, default),
                             local folder only, or cache and lrclib only
      --lyrics-dir PATH      folder with .lrc files (default: ~/Lyrics)

Player:
      --player NAME          follow only this player (spotify, mpv...)
      --list-players         list MPRIS players and exit

What to show:
      --no-header            hide the whole header
      --no-artist            show only the title in the header
      --no-source            hide the player name in the header
      --no-missing           hide the "missing in font" hint
      --clean                same as --no-artist --no-source --no-missing
      --prev                 previous line above, dimmed
      --next                 next line below, dimmed
      --mode line|word       whole lines or one word at a time

Files:
      --export PATH          write the current line to a file (for status bars)
      --save-lrc DIR         save found .lrc files into a directory

Rendering:
      --renderer block|kitty block font or an image in kitty
      --no-bold              no bold weight (block only)
      --style bold|mono|soft font style (kitty)
      --font PATH            custom TTF font (kitty)
      --font-size N          font size in pixels (kitty, default: 56)
      --spacing N            letter spacing in pixels (kitty)
      --glow / --no-glow     glow effect (kitty)

While running:
  + / -    shift sync by 0.1 s
  0        reset the shift
  v        switch to the next synced version of the track (if lrclib has one)
  q        quit

  -h, --help                 show this help
)";
}

Options parse_args(int argc, char** argv) {
    Options o;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        std::string inline_val;
        if (a.rfind("--", 0) == 0) {
            auto eq = a.find('=');
            if (eq != std::string::npos) {
                inline_val = a.substr(eq + 1);
                a = a.substr(0, eq);
            }
        }
        auto need = [&]() -> std::string {
            if (!inline_val.empty()) return inline_val;
            if (i + 1 >= argc) throw std::runtime_error("option " + a + " needs a value");
            return argv[++i];
        };

        if (a == "-c" || a == "--color") o.color = need();
        else if (a == "--from-kitty") o.from_kitty = need();
        else if (a == "--player") o.player = util::lower(need());
        else if (a == "--list-players") o.list_players = true;
        else if (a == "--no-header") o.no_header = true;
        else if (a == "--no-artist") o.no_artist = true;
        else if (a == "--no-source") o.no_source = true;
        else if (a == "--no-missing") o.no_missing = true;
        else if (a == "--clean") o.no_artist = o.no_source = o.no_missing = true;
        else if (a == "--align") o.left = need() == "left";
        else if (a == "--vpos") o.vpos = std::stoi(need());
        else if (a == "--no-bold") o.no_bold = true;
        else if (a == "--mode") o.word_mode = need() == "word";
        else if (a == "--renderer") o.kitty = need() == "kitty";
        else if (a == "--rainbow") o.rainbow = true;
        else if (a == "--prev") o.prev_line = true;
        else if (a == "--next") o.next_line = true;
        else if (a == "--dim-paused") o.dim_paused = true;
        else if (a == "--fade") o.fade = true;
        else if (a == "--headless") o.headless = true;
        else if (a == "--backend") {
            o.backend = need();
            if (o.backend != "auto" && o.backend != "local" && o.backend != "lrclib")
                throw std::runtime_error("--backend must be auto, local or lrclib");
        }
        else if (a == "--lyrics-dir") o.lyrics_dir = need();
        else if (a == "--export") o.export_path = need();
        else if (a == "--save-lrc") o.save_lrc_dir = need();
        else if (a == "--style") o.style = need();
        else if (a == "--font") o.font = need();
        else if (a == "--font-size") o.font_size = std::stoi(need());
        else if (a == "--spacing") o.spacing = std::stoi(need());
        else if (a == "--glow") o.glow = 1;
        else if (a == "--no-glow") o.glow = 0;
        else if (a == "-h" || a == "--help") {
            print_help();
            std::exit(0);
        } else {
            throw std::runtime_error("unknown option: " + a + " (see --help)");
        }
    }
    return o;
}

struct Style {
    std::vector<std::string> fonts;
    int spacing;
    bool glow;
};

Style get_style(const std::string& name) {
    if (name == "bold")
        return {{"/usr/share/fonts/noto/NotoSans-Bold.ttf", "/usr/share/fonts/TTF/DejaVuSans-Bold.ttf"}, 2, false};
    if (name == "mono")
        return {{"/usr/share/fonts/noto/NotoSansMono-Bold.ttf", "/usr/share/fonts/TTF/DejaVuSansMono-Bold.ttf"}, 4, false};
    if (name == "soft")
        return {{"/usr/share/fonts/noto/NotoSans-SemiBold.ttf", "/usr/share/fonts/noto/NotoSans-Bold.ttf",
                 "/usr/share/fonts/TTF/DejaVuSans-Bold.ttf"},
                3, true};
    throw std::runtime_error("unknown style: " + name + " (bold, mono, soft)");
}

std::string resolve_font(const Options& o, const Style& st) {
    if (!o.font.empty()) {
        if (access(o.font.c_str(), R_OK) == 0) return o.font;
        throw std::runtime_error("font not found: " + o.font);
    }
    std::vector<std::string> candidates = st.fonts;
    candidates.push_back("/usr/share/fonts/noto/NotoSans-Regular.ttf");
    candidates.push_back("/usr/share/fonts/TTF/DejaVuSans.ttf");
    for (const auto& p : candidates)
        if (access(p.c_str(), R_OK) == 0) return p;
    throw std::runtime_error("no font with Cyrillic found; install noto-fonts and ttf-dejavu");
}

// Ctrl+C: restore the terminal, remove kitty images and exit
void on_sigint(int) {
    input::restore();
    const char* clear = "\x1b_Ga=d,d=a,q=2\x1b\\\033[0m\n";
    if (write(STDOUT_FILENO, clear, std::strlen(clear)) < 0) {
    }
    _exit(0);
}

// Atomic write of the current line: write a temp file, then rename it
void write_export(const std::string& path, const std::string& text) {
    if (path.empty()) return;
    std::string tmp = path + ".tmp";
    {
        std::ofstream f(tmp, std::ios::binary | std::ios::trunc);
        f << text;
    }
    std::rename(tmp.c_str(), path.c_str());
}

// Background lyric lookup. The result is collected once it is ready
struct Job {
    std::string key, artist, title;
    std::future<std::optional<std::string>> result;
};

std::string fmt_offset(double v) {
    char buf[32];
    std::snprintf(buf, sizeof buf, "%+.1f s", v);
    return buf;
}

}  // namespace

int main(int argc, char** argv) {
    try {
        Options o = parse_args(argc, argv);
        util::g_vpos = std::clamp(o.vpos, 0, 100);
        if (o.lyrics_dir.empty()) {
            const char* home = std::getenv("HOME");
            o.lyrics_dir = home ? std::string(home) + "/Lyrics" : std::string();
        }

        if (o.list_players) {
            Mpris m;
            auto names = m.list_players();
            if (names.empty()) std::cout << "no MPRIS players found\n";
            for (const auto& n : names) std::cout << n << "\n";
            return 0;
        }

        Style st = get_style(o.style);
        int spacing = o.spacing >= 0 ? o.spacing : st.spacing;
        bool glow = o.glow < 0 ? st.glow : o.glow == 1;

        bool use_kitty = o.kitty;
        if (use_kitty && !util::in_kitty()) {
            std::cerr << "kitty not detected, falling back to the block renderer\n";
            use_kitty = false;
        }

        util::Palette palette;
        if (use_kitty || !o.from_kitty.empty()) palette = util::kitty_palette();

        auto parsed = util::resolve_color(o.from_kitty.empty() ? o.color : o.from_kitty, palette);
        if (!parsed) {
            std::cerr << "unknown color, using cyan\n";
            parsed = util::Rgb{0, 205, 205};
        }
        const util::Rgb base = *parsed;

        std::unique_ptr<kitty::Renderer> kr;
        if (use_kitty) kr = std::make_unique<kitty::Renderer>(resolve_font(o, st), o.font_size, spacing, glow);

        curl_global_init(CURL_GLOBAL_DEFAULT);
        if (!o.headless) input::enable_raw();  // no raw terminal in headless mode, so it can run in the background
        std::atexit(input::restore);
        std::signal(SIGINT, on_sigint);
        std::signal(SIGTERM, on_sigint);

        Mpris mpris;
        std::map<std::string, std::vector<LyricLine>> mem;  // parsed lyrics by track key
        std::vector<Job> jobs;                              // lookups in progress
        std::string cur_key;
        std::string cur_artist, cur_title;
        std::vector<LyricLine> lines;
        double offset = 0.0;  // sync shift in seconds

        // Other synced versions of the current track, loaded on the first 'v' press
        std::vector<std::string> variants;
        bool variants_loaded = false;
        size_t variant_idx = 0;
        std::future<std::optional<std::vector<std::string>>> variant_job;  // background 'v' lookup
        bool variant_pending = false;
        std::string variant_note;  // header hint: "versions unavailable" or "no other versions"

        // Redraw markers: -2 forces a redraw, -3 no lyrics, -4 loading, otherwise the line index
        long last = -2;
        bool last_paused = false;

        // Easter egg: type "lyrics" while the program runs for ten seconds of rainbow colors
        std::string key_buf;
        std::chrono::steady_clock::time_point secret_until{};
        bool last_secret = false;

        // opacity: 1.0 is normal, lower values dim the text (pause, fade-in frames)
        auto draw = [&](const std::string& header, const std::string& text, const std::string& note,
                        const std::string& prev, const std::string& next, util::Rgb rgb, float opacity) {
            if (o.headless) return;  // no terminal output in headless mode
            if (use_kitty) kitty::draw(*kr, text, header, note, prev, next, rgb, opacity, o.left);
            else block::draw(text, header, note, prev, next, rgb, !o.no_bold, opacity < 1.0f, o.left);
        };

        const std::string waiting = "Waiting for a music player (MPRIS)...";
        draw("", "", waiting, "", "", base, 1.0f);

        while (true) {
            // 1. Keys: sync shift, version switch, quit
            char k = input::poll_key();
            if (k == 'q' || k == 'Q') break;
            if (k) {
                key_buf += k;
                if (key_buf.size() > 16) key_buf.erase(0, key_buf.size() - 16);
                if (key_buf.size() >= 6 && key_buf.compare(key_buf.size() - 6, 6, "lyrics") == 0) {
                    secret_until = std::chrono::steady_clock::now() + std::chrono::seconds(10);
                    key_buf.clear();
                }
            }
            if (k == '+' || k == '=') { offset += 0.1; last = -2; }
            else if (k == '-' || k == '_') { offset -= 0.1; last = -2; }
            else if (k == '0') { offset = 0.0; last = -2; }
            else if (k == 'v' || k == 'V') {
                if (!variants_loaded && !cur_artist.empty()) {
                    // Lookup runs in the background; the result is applied in step 1b below
                    variant_job = std::async(std::launch::async, [t = cur_artist, s = cur_title]() {
                        return lyrics::fetch_candidates(t, s);
                    });
                    variant_pending = true;
                    variant_note = "looking for versions...";
                    variants_loaded = true;
                    variant_idx = 0;
                } else if (!variants.empty()) {
                    variant_idx = (variant_idx + 1) % variants.size();
                    lines = lyrics::build_units(lyrics::parse_lrc(variants[variant_idx]), o.word_mode);
                    last = -2;
                }
            }

            // 1b. Finished background version lookup
            if (variant_pending && variant_job.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
                variant_pending = false;
                auto found = variant_job.get();
                variants = found ? *found : std::vector<std::string>{};
                if (!found) variant_note = "versions unavailable";
                else if (variants.size() <= 1) variant_note = "no other versions";
                else variant_note.clear();
                if (!variants.empty()) {
                    lines = lyrics::build_units(lyrics::parse_lrc(variants[0]), o.word_mode);
                    last = -2;
                }
            }

            // 2. Finished lookups
            for (size_t i = 0; i < jobs.size();) {
                if (jobs[i].result.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
                    ++i;
                    continue;
                }
                auto res = jobs[i].result.get();
                if (res) {
                    mem[jobs[i].key] = lyrics::parse_lrc(*res);
                    if (!res->empty() && !o.save_lrc_dir.empty())
                        lyrics::save_lrc(o.save_lrc_dir, jobs[i].artist, jobs[i].title, *res);
                }
                if (jobs[i].key == cur_key) {
                    if (res) lines = lyrics::build_units(mem[jobs[i].key], o.word_mode);
                    else lines.clear();
                    last = -2;
                }
                jobs.erase(jobs.begin() + static_cast<long>(i));
            }

            // 3. Current player
            auto p = mpris.best(o.player);
            if (!p) {
                if (!cur_key.empty()) {
                    cur_key.clear();
                    cur_artist.clear();
                    write_export(o.export_path, "");
                    cur_title.clear();
                    lines.clear();
                    variants.clear();
                    variants_loaded = false;
                    offset = 0.0;
                    last = -2;
                    draw("", "", waiting, "", "", base, 1.0f);
                }
                std::this_thread::sleep_for(std::chrono::seconds(1));
                continue;
            }

            // 4. Track change: use memory or start a background lookup
            std::string key = p->artist + "\x1f" + p->title;
            if (key != cur_key) {
                cur_key = key;
                cur_artist = p->artist;
                cur_title = p->title;
                variants.clear();
                variants_loaded = false;
                variant_pending = false;  // a late result for the old track is ignored
                variant_note.clear();
                variant_idx = 0;
                last = -2;
                offset = 0.0;
                auto it = mem.find(key);
                if (it != mem.end()) {
                    lines = lyrics::build_units(it->second, o.word_mode);
                } else {
                    lines.clear();
                    bool queued = std::any_of(jobs.begin(), jobs.end(), [&](const Job& j) { return j.key == key; });
                    if (!queued) {
                        Job j;
                        j.key = key;
                        j.artist = p->artist;
                        j.title = p->title;
                        std::string artist = p->artist, title = p->title;
                        j.result = std::async(std::launch::async,
                            [artist, title, backend = o.backend, dir = o.lyrics_dir]() -> std::optional<std::string> {
                                if (artist.empty()) return std::string();
                                return lyrics::load_track(artist, title, backend, dir);
                            });
                        jobs.push_back(std::move(j));
                    }
                }
            }

            bool loading = std::any_of(jobs.begin(), jobs.end(), [&](const Job& j) { return j.key == cur_key; });
            bool paused = p->status == "Paused" || p->status == "Stopped";

            bool secret = std::chrono::steady_clock::now() < secret_until;
            if (secret != last_secret) {
                last_secret = secret;
                last = -2;  // redraw when the effect starts or ends
            }

            std::string header;
            if (!o.no_header) {
                header = "🎵 ";
                header += o.no_artist ? p->title : p->artist + " — " + p->title;
                if (!o.no_source) header += "  · " + p->name;
                if (offset != 0.0) header += "  · shift " + fmt_offset(offset);
                if (secret) header += "  · ♪";
                if (variants_loaded && variants.size() > 1)
                    header += "  · version " + std::to_string(variant_idx + 1) + "/" + std::to_string(variants.size());
                else if (!variant_note.empty())
                    header += "  · " + variant_note;
            }

            // 5. Output
            if (loading) {
                if (last != -4) {
                    last = -4;
                    write_export(o.export_path, "");
                    draw(header, "", "loading lyrics...", "", "", base, 1.0f);
                }
            } else if (lines.empty()) {
                if (last != -3) {
                    last = -3;
                    write_export(o.export_path, "");
                    draw(header, "", "✕ Lyrics not found", "", "", base, 1.0f);
                }
            } else {
                double pos = p->position + offset;
                auto ub = std::upper_bound(lines.begin(), lines.end(), pos,
                                           [](double t, const LyricLine& l) { return t < l.time; });
                long idx = static_cast<long>(ub - lines.begin()) - 1;

                if (idx != last || paused != last_paused) {
                    last = idx;
                    last_paused = paused;

                    std::string text;
                    if (idx >= 0) text = lines[static_cast<size_t>(idx)].text;

                    std::string prev;
                    if (o.prev_line && idx >= 1) prev = lines[static_cast<size_t>(idx - 1)].text;

                    std::string next;
                    if (o.next_line && idx + 1 < static_cast<long>(lines.size()))
                        next = lines[static_cast<size_t>(idx + 1)].text;

                    util::Rgb line_rgb = base;
                    if ((o.rainbow || secret) && idx >= 0)
                        line_rgb = util::hsv_to_rgb(static_cast<double>(idx) * 35.0, 0.75, 1.0);

                    std::string note;
                    if (!use_kitty && !o.no_missing) note = block::missing_note(text);

                    float base_opacity = (o.dim_paused && paused) ? 0.45f : 1.0f;
                    write_export(o.export_path, text);

                    if (o.fade && use_kitty) {
                        // Four frames of increasing opacity: a short fade-in of the new line
                        for (float f : {0.25f, 0.5f, 0.75f, 1.0f}) {
                            draw(header, text, note, prev, next, line_rgb, f * base_opacity);
                            std::this_thread::sleep_for(std::chrono::milliseconds(60));
                        }
                    } else {
                        draw(header, text, note, prev, next, line_rgb, base_opacity);
                    }
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    } catch (const std::exception& e) {
        input::restore();
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
