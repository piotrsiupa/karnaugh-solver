#include <iostream>
#include <string>

#include "global.hh"
#include "input.hh"
#include "Karnaugh.hh"


int main()
{
	lines_t lines;
	if (!loadLines(std::cin, lines))
		return 1;
		
	if (lines.empty())
	{
		std::cerr << "Input names are missing!\n";
		return 1;
	}
	const names_t names = readStrings(lines.front());
	if (names.size() > maxBits)
	{
		std::cerr << "Too many variables!\n";
		return false;
	}
	lines.pop_front();
	
	const bool result = Karnaugh::processMultiple(names, lines);
	return result ? 0 : 1;
}
