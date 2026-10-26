// Unit tests for the parts of lyrics.hpp that do not need a network or a player.

#include <cstdio>

#include "lyrics.hpp"

namespace {

int failures = 0;

#define CHECK(cond)                                                      \
    do {                                                                 \
        if (!(cond)) {                                                   \
            std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
            ++failures;                                                  \
        }                                                                \
    } while (0)

void test_parse_lrc() {
    // Two timestamps on one line produce two entries with the same text
    auto a = lyrics::parse_lrc("[00:01.00][00:05.00] hello\n[00:03.50] world\nnot a lyric line\n[ar:someone]\n");
    CHECK(a.size() == 3);
    if (a.size() == 3) {
        CHECK(a[0].time == 1.0 && a[0].text == "hello");
        CHECK(a[1].time == 3.5 && a[1].text == "world");
        CHECK(a[2].time == 5.0 && a[2].text == "hello");
    }
}

void test_parse_lrc_empty() {
    CHECK(lyrics::parse_lrc("").empty());
    CHECK(lyrics::parse_lrc("no timestamps here\n").empty());
}

void test_word_mode() {
    // Line 1 spans 0..4 seconds and has two words: 0.0 and 2.0.
    // Line 2 has no following line, so it gets a 3-second span.
    auto w = lyrics::build_units({{0.0, "one two"}, {4.0, "three"}}, true);
    CHECK(w.size() == 3);
    if (w.size() == 3) {
        CHECK(w[0].text == "one" && w[0].time == 0.0);
        CHECK(w[1].text == "two" && w[1].time == 2.0);
        CHECK(w[2].text == "three" && w[2].time == 4.0);
    }
}

void test_line_mode_unchanged() {
    std::vector<LyricLine> in = {{1.0, "a b"}, {2.0, "c"}};
    auto out = lyrics::build_units(in, false);
    CHECK(out.size() == in.size());
}

void test_safe_name() {
    CHECK(lyrics::safe_name("a/b:c") == "a_b_c");
    CHECK(lyrics::safe_name("plain name") == "plain name");
}

}  // namespace

int main() {
    test_parse_lrc();
    test_parse_lrc_empty();
    test_word_mode();
    test_line_mode_unchanged();
    test_safe_name();

    if (failures) {
        std::printf("%d check(s) failed\n", failures);
        return 1;
    }
    std::printf("all tests passed\n");
    return 0;
}
