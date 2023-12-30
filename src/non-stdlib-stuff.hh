#pragma once

#if __has_include(<unistd.h>)
#include <unistd.h>
#endif


#if __has_include(<unistd.h>)
inline bool isStdinTerminal()
{
	return isatty(STDIN_FILENO);
}
inline bool isStderrTerminal()
{
	return isatty(STDERR_FILENO);
}
#elif defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
bool isStdinTerminal();
bool isStderrTerminal();
#else
inline bool isStdinTerminal()
{
	return true; // Fallback - just assume tty and print the hints.
}
inline bool isStderrTerminal()
{
	return true; // Fallback - just assume tty and print the progress reports.
}
#endif

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
void enableAnsiSequences();
#else
inline void enableAnsiSequences() {}
#endif
