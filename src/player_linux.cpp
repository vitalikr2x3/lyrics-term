// Linux player source: MPRIS over the session D-Bus (see mpris.hpp).
#include "platform.hpp"
#include "mpris.hpp"

std::optional<PlayerInfo> best_player(const std::string& wanted) {
    static Mpris mpris;  // one D-Bus connection for the whole run
    return mpris.best(wanted);
}

std::vector<std::string> list_players() {
    static Mpris mpris;
    return mpris.list_players();
}
