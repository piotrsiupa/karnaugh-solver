#pragma once

#include <atomic>
#include <list>
#include <string>
#include <vector>


using lines_t = std::list<std::string>;
using names_t = std::vector<std::string>;


extern std::atomic<bool> intSignalFlag;
