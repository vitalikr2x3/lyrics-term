// Windows player source: System Media Transport Controls (SMTC), the session list Windows shows in the media flyout.
// Written against C++/WinRT; the method names are checked only on a real Windows build.
#include "platform.hpp"

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Media.Control.h>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <string>
#include <vector>

using namespace winrt::Windows::Media::Control;

namespace {

std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

// COM must be initialized once per thread before WinRT calls
void init_com() {
    static const bool done = [] {
        winrt::init_apartment(winrt::apartment_type::multi_threaded);
        return true;
    }();
    (void)done;
}

GlobalSystemMediaTransportControlsSessionManager manager() {
    init_com();
    return GlobalSystemMediaTransportControlsSessionManager::RequestAsync().get();
}

}  // namespace

std::optional<PlayerInfo> best_player(const std::string& wanted) {
    std::optional<PlayerInfo> chosen;
    for (auto session : manager().GetSessions()) {
        const std::string app = winrt::to_string(session.SourceAppUserModelId());
        if (!wanted.empty() && lower(app).find(wanted) == std::string::npos) continue;

        auto media = session.TryGetMediaPropertiesAsync().get();
        PlayerInfo p;
        p.name = app;
        p.title = winrt::to_string(media.Title());
        if (p.title.empty()) continue;
        p.artist = winrt::to_string(media.Artist());

        const bool playing =
            session.GetPlaybackInfo().PlaybackStatus() == GlobalSystemMediaTransportControlsSessionPlaybackStatus::Playing;
        p.status = playing ? "Playing" : "Paused";

        // SMTC reports the position at its last update; while playing, add the time since then
        auto timeline = session.GetTimelineProperties();
        double pos = std::chrono::duration<double>(timeline.Position()).count();
        if (playing) {
            pos += std::chrono::duration<double>(winrt::clock::now() - timeline.LastUpdatedTime()).count();
        }
        p.position = pos;

        if (!chosen || (playing && chosen->status != "Playing")) chosen = p;
    }
    return chosen;
}

std::vector<std::string> list_players() {
    std::vector<std::string> out;
    for (auto session : manager().GetSessions()) {
        out.push_back(winrt::to_string(session.SourceAppUserModelId()));
    }
    return out;
}
