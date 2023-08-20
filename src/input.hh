#pragma once

#include <istream>
#include <string>

#include "global.hh"


bool loadLines(std::istream &in, lines_t &lines);

strings_t readStrings(std::string &line);
