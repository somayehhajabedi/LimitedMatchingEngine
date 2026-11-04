#pragma once

#include <iosfwd>

namespace matching {

// Processes one command per line; ignores malformed commands.
void processCommands(std::istream& input, std::ostream& output);

} // namespace matching
