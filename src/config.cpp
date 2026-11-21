#include "config.hpp"

#include "paths.hpp"

#include <kdl/parser.h>

#include <cstdio>
#include <fstream>
#include <memory>
#include <sstream>
#include <stdexcept>

namespace config {
namespace {

struct Frame {
    std::string name;
    std::string arg;          // first argument, as text
    bool has_arg = false;
    bool is_bool = false;     // the argument was #true or #false
    bool has_children = false;
};

std::string value_to_string(const kdl_value& v) {
    switch (v.type) {
    case KDL_TYPE_STRING:
        return std::string(v.string.data, v.string.len);
    case KDL_TYPE_BOOLEAN:
        return v.boolean ? "true" : "false";
    case KDL_TYPE_NUMBER:
        if (v.number.type == KDL_NUMBER_TYPE_INTEGER) return std::to_string(v.number.integer);
        if (v.number.type == KDL_NUMBER_TYPE_FLOATING_POINT) {
            char buf[64];
            std::snprintf(buf, sizeof buf, "%g", v.number.floating_point);
            return buf;
        }
        return std::string(v.number.string.data, v.number.string.len);
    default:
        return "";
    }
}

}  // namespace

std::vector<std::string> flags_from_file(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return {};
    std::stringstream ss;
    ss << in.rdbuf();
    const std::string text = ss.str();

    std::unique_ptr<kdl_parser, void (*)(kdl_parser*)> parser(
        kdl_create_string_parser(kdl_str{text.data(), text.size()}, KDL_DEFAULTS), kdl_destroy_parser);
    if (!parser) throw std::runtime_error(path + ": cannot start the KDL parser");

    std::vector<std::string> out;
    std::vector<Frame> stack;
    for (;;) {
        const kdl_event_data* ev = kdl_parser_next_event(parser.get());
        switch (ev->event) {
        case KDL_EVENT_EOF:
            return out;
        case KDL_EVENT_PARSE_ERROR:
            throw std::runtime_error(path + ": syntax error (check quotes, braces and #true/#false)");
        case KDL_EVENT_START_NODE:
            if (!stack.empty()) stack.back().has_children = true;
            stack.emplace_back();
            stack.back().name = std::string(ev->name.data, ev->name.len);
            break;
        case KDL_EVENT_ARGUMENT:
            if (!stack.empty() && !stack.back().has_arg) {
                stack.back().has_arg = true;
                stack.back().is_bool = ev->value.type == KDL_TYPE_BOOLEAN;
                stack.back().arg = value_to_string(ev->value);
            }
            break;
        case KDL_EVENT_END_NODE: {
            if (stack.empty()) break;
            Frame f = std::move(stack.back());
            stack.pop_back();
            if (f.has_children || !f.has_arg) break;  // a section or a bare node: no flag
            if (f.is_bool) {
                if (f.arg == "true") out.push_back("--" + f.name);
            } else {
                out.push_back("--" + f.name + "=" + f.arg);
            }
            break;
        }
        default:
            break;  // properties and comments are not used
        }
    }
}

std::vector<std::string> merged_args(int argc, char** argv) {
    std::string explicit_path;
    bool disabled = false;
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        if (a == "--no-config") {
            disabled = true;
        } else if (a == "--config" && i + 1 < argc) {
            explicit_path = argv[++i];
        } else if (a.rfind("--config=", 0) == 0) {
            explicit_path = a.substr(9);
        }
    }

    std::vector<std::string> out;
    out.push_back(argc > 0 ? argv[0] : "lyrics-term");
    if (!disabled) {
        std::string path = explicit_path.empty() ? config_dir() + "/config.kdl" : explicit_path;
        if (!explicit_path.empty() && !file_readable(path)) {
            throw std::runtime_error("config file not found: " + path);
        }
        for (auto& f : flags_from_file(path)) out.push_back(f);
    }
    for (int i = 1; i < argc; ++i) out.push_back(argv[i]);
    return out;
}

}  // namespace config
