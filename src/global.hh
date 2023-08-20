#pragma once

#include <cstdint>
#include <list>
#include <string>
#include <vector>


using lines_t = std::list<std::string>;

using strings_t = std::vector<std::string>;

extern strings_t inputNames;

using bits_t = std::uint_fast8_t;
constexpr bits_t maxBits = 32;
extern bits_t bits;
