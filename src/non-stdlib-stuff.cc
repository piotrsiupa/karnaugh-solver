#include "./non-stdlib-stuff.hh"

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
// This header is polluting the global namespace at an incredible degree.
// It had to be banished to a separate file to not break everything.
#include <Windows.h>
#endif


#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
bool isInputTerminal()
{
	const HANDLE inputHandle = GetStdHandle(STD_INPUT_HANDLE);
	if (inputHandle == INVALID_HANDLE_VALUE)
		return false;
	DWORD consoleMode;
	return GetConsoleMode(inputHandle, &consoleMode);
}
#endif