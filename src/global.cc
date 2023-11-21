#include "./global.hh"


bool terminalStdin, terminalInput, terminalStderr;

std::optional<std::string_view> inputFilePath;
Names inputNames;

bits_t bits;
std::uint_fast32_t maxMinterm;
