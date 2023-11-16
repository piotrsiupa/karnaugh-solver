#include "./global.hh"


bool terminalStdin, terminalInput, terminalStderr;

std::optional<std::string_view> inputFilePath;
std::vector<std::string> inputNames;

bits_t bits;
std::uint_fast32_t maxMinterm;
