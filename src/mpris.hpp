#pragma once
// MPRIS access over the session D-Bus (libdbus).

#include <dbus/dbus.h>

#include <cstdint>
#include <cstring>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include "util.hpp"

struct PlayerInfo {
    std::string name;     // short name: spotify, mpv, audacious
    std::string artist;
    std::string title;
    std::string status;   // Playing / Paused / Stopped
    double position = 0;  // seconds
};

class Mpris {
public:
    Mpris() {
        DBusError err;
        dbus_error_init(&err);
        conn_ = dbus_bus_get(DBUS_BUS_SESSION, &err);
        if (!conn_) {
            std::string msg = err.message ? err.message : "unknown error";
            clear_err(err);
            throw std::runtime_error("D-Bus is not available: " + msg);
        }
    }

    // Short names of all running MPRIS players
    std::vector<std::string> list_players() const {
        std::vector<std::string> out;
        DBusMessage* msg = dbus_message_new_method_call("org.freedesktop.DBus", "/org/freedesktop/DBus",
                                                        "org.freedesktop.DBus", "ListNames");
        if (!msg) return out;
        DBusError err;
        dbus_error_init(&err);
        DBusMessage* reply = dbus_connection_send_with_reply_and_block(conn_, msg, 500, &err);
        dbus_message_unref(msg);
        clear_err(err);
        if (!reply) return out;

        DBusMessageIter it, arr;
        if (dbus_message_iter_init(reply, &it) && dbus_message_iter_get_arg_type(&it) == DBUS_TYPE_ARRAY) {
            dbus_message_iter_recurse(&it, &arr);
            while (dbus_message_iter_get_arg_type(&arr) == DBUS_TYPE_STRING) {
                std::string n = basic_str(&arr);
                if (n.rfind(kPrefix, 0) == 0) out.push_back(n.substr(std::strlen(kPrefix)));
                dbus_message_iter_next(&arr);
            }
        }
        dbus_message_unref(reply);
        return out;
    }

    // Picks a player: a playing one wins. wanted is a lowercase substring of the name, or empty
    std::optional<PlayerInfo> best(const std::string& wanted) const {
        std::optional<PlayerInfo> chosen;
        for (const auto& n : list_players()) {
            if (!wanted.empty() && util::lower(n).find(wanted) == std::string::npos) continue;
            auto p = read(n);
            if (!p) continue;
            if (!chosen || (p->status == "Playing" && chosen->status != "Playing")) chosen = std::move(p);
        }
        return chosen;
    }

private:
    static constexpr const char* kPrefix = "org.mpris.MediaPlayer2.";
    DBusConnection* conn_ = nullptr;

    static void clear_err(DBusError& e) {
        if (dbus_error_is_set(&e)) dbus_error_free(&e);
    }

    static std::string basic_str(DBusMessageIter* it) {
        const char* s = nullptr;
        dbus_message_iter_get_basic(it, &s);
        return s ? s : "";
    }

    // Opens the variant returned by Properties.Get
    static bool open_variant(DBusMessage* m, DBusMessageIter* inner) {
        DBusMessageIter it;
        if (!dbus_message_iter_init(m, &it)) return false;
        if (dbus_message_iter_get_arg_type(&it) != DBUS_TYPE_VARIANT) return false;
        dbus_message_iter_recurse(&it, inner);
        return true;
    }

    DBusMessage* get_prop(const std::string& name, const char* prop) const {
        std::string bus = std::string(kPrefix) + name;
        DBusMessage* msg = dbus_message_new_method_call(bus.c_str(), "/org/mpris/MediaPlayer2",
                                                        "org.freedesktop.DBus.Properties", "Get");
        if (!msg) return nullptr;
        const char* iface = "org.mpris.MediaPlayer2.Player";
        dbus_message_append_args(msg, DBUS_TYPE_STRING, &iface, DBUS_TYPE_STRING, &prop, DBUS_TYPE_INVALID);

        DBusError err;
        dbus_error_init(&err);
        DBusMessage* reply = dbus_connection_send_with_reply_and_block(conn_, msg, 300, &err);
        dbus_message_unref(msg);
        clear_err(err);
        if (reply && dbus_message_get_type(reply) == DBUS_MESSAGE_TYPE_ERROR) {
            dbus_message_unref(reply);
            return nullptr;
        }
        return reply;
    }

    static void read_metadata(DBusMessageIter* dict, PlayerInfo& info) {
        while (dbus_message_iter_get_arg_type(dict) == DBUS_TYPE_DICT_ENTRY) {
            DBusMessageIter entry, val;
            dbus_message_iter_recurse(dict, &entry);
            std::string key = basic_str(&entry);
            dbus_message_iter_next(&entry);

            if (dbus_message_iter_get_arg_type(&entry) == DBUS_TYPE_VARIANT) {
                dbus_message_iter_recurse(&entry, &val);
                int t = dbus_message_iter_get_arg_type(&val);
                if (key == "xesam:title" && t == DBUS_TYPE_STRING) {
                    info.title = basic_str(&val);
                } else if (key == "xesam:artist" && t == DBUS_TYPE_ARRAY) {
                    DBusMessageIter arr;
                    dbus_message_iter_recurse(&val, &arr);
                    if (dbus_message_iter_get_arg_type(&arr) == DBUS_TYPE_STRING)
                        info.artist = basic_str(&arr);
                }
            }
            dbus_message_iter_next(dict);
        }
    }

    std::optional<PlayerInfo> read(const std::string& name) const {
        PlayerInfo info;
        info.name = name;

        if (DBusMessage* m = get_prop(name, "Metadata")) {
            DBusMessageIter var;
            if (open_variant(m, &var) && dbus_message_iter_get_arg_type(&var) == DBUS_TYPE_ARRAY) {
                DBusMessageIter dict;
                dbus_message_iter_recurse(&var, &dict);
                read_metadata(&dict, info);
            }
            dbus_message_unref(m);
        }
        if (info.title.empty()) return std::nullopt;

        if (DBusMessage* m = get_prop(name, "PlaybackStatus")) {
            DBusMessageIter var;
            if (open_variant(m, &var) && dbus_message_iter_get_arg_type(&var) == DBUS_TYPE_STRING)
                info.status = basic_str(&var);
            dbus_message_unref(m);
        }
        if (DBusMessage* m = get_prop(name, "Position")) {
            DBusMessageIter var;
            if (open_variant(m, &var) && dbus_message_iter_get_arg_type(&var) == DBUS_TYPE_INT64) {
                dbus_int64_t us = 0;
                dbus_message_iter_get_basic(&var, &us);
                info.position = static_cast<double>(us) / 1e6;
            }
            dbus_message_unref(m);
        }
        return info;
    }
};
