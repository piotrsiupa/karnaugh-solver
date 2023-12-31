#pragma once

#include <cstdint>
#include <list>
#include <optional>
#include <string_view>

#include "Names.hh"


extern bool terminalStdin, terminalInput, terminalStderr;

extern std::optional<std::string_view> inputFilePath;
extern Names inputNames;

using bits_t = std::uint_fast8_t;
constexpr bits_t maxBits = 32;
extern bits_t bits;
extern std::uint_fast32_t maxMinterm;
