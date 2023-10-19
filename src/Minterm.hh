#pragma once

#include <climits>
#include <cstdint>

#include "global.hh"


using Minterm = std::uint_fast32_t;
static_assert(sizeof(Minterm) * CHAR_BIT >= maxBits);
