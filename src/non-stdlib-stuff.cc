#include "./non-stdlib-stuff.hh"

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
// This header is polluting the global namespace at an incredible degree.
// It had to be banished to a separate file to not break everything.
#include <Windows.h>
#endif


#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
static bool isTerminal(const DWORD stream)
{
	const HANDLE handle = GetStdHandle(stream);
	if (handle == INVALID_HANDLE_VALUE)
		return false;
	DWORD consoleMode;
	return GetConsoleMode(handle, &consoleMode);
}
bool isStdinTerminal()
{
	return isTerminal(STD_INPUT_HANDLE);
}
bool isStderrTerminal()
{
	return isTerminal(STD_ERROR_HANDLE);
}
#endif
