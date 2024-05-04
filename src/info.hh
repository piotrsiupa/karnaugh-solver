#pragma once

#include <string_view>


std::string_view getInternalName();
std::string_view getFullName();
std::string_view getVersionNumber();
std::string_view getAuthor();

void printShortHelp();
void printHelp();
void printVersion();
