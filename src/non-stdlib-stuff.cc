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

static void enableAnsiSequences(const DWORD stream)
{
	const HANDLE handle = GetStdHandle(stream);
	DWORD consoleMode;
	if (handle != INVALID_HANDLE_VALUE && GetConsoleMode(handle, &consoleMode))
	{
		consoleMode |= ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING;
		SetConsoleMode(handle, consoleMode);
	}
}

void enableAnsiSequences()
{
	enableAnsiSequences(STD_OUTPUT_HANDLE);
	enableAnsiSequences(STD_ERROR_HANDLE);
}
#endif
