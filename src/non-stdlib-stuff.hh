#pragma once

#if __has_include(<unistd.h>)
#include <unistd.h>
#endif


#if __has_include(<unistd.h>)
inline bool isInputTerminal()
{
	return isatty(STDIN_FILENO);
}
#elif defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
bool isInputTerminal();
#else
inline bool isInputTerminal()
{
	return true; // Fallback - just assume tty and print all the hints.
}
#endif
