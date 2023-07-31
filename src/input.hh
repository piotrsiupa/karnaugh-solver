#pragma once

#include <istream>
#include <string>
#include <vector>

#include "global.hh"


using strings_t = std::vector<std::string>;

bool loadLines(std::istream &in, lines_t &lines);

strings_t readStrings(std::string &line);
