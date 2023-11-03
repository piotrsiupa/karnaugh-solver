#pragma once

#include <cstdint>
#include <list>
#include <string>
#include <vector>


extern bool terminalStdin, terminalStderr;

extern std::vector<std::string> inputNames;

using bits_t = std::uint_fast8_t;
constexpr bits_t maxBits = 32;
extern bits_t bits;
extern std::uint_fast32_t maxMinterm;
