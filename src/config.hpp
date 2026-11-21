#pragma once
// config.kdl support. The file is turned into command-line flags, so every option
// that works on the command line works in the file too. Order: config file first,
// command line second, so the command line wins.

#include <string>
#include <vector>

namespace config {

// argv with the config flags inserted after argv[0]. Throws std::runtime_error if the
// config file cannot be read or parsed, or if --config points to a missing file
std::vector<std::string> merged_args(int argc, char** argv);

// Flags from one KDL file: a leaf node "name value" becomes "--name=value",
// "name #true" becomes "--name", "name #false" is skipped. Sections (nodes with children)
// are only containers. Throws std::runtime_error on a syntax error
std::vector<std::string> flags_from_file(const std::string& path);

}  // namespace config
