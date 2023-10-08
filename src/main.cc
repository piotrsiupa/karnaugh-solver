#include <iostream>
#include <string>

#include "global.hh"
#include "input.hh"
#include "Karnaughs.hh"


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
	::inputNames = readStrings(lines.front());
	if (::inputNames.size() > ::maxBits)
	{
		std::cerr << "Too many variables!\n";
		return false;
	}
	lines.pop_front();
	
	::bits = ::inputNames.size();
	
	if (!Karnaughs::solve(lines))
		return 1;
	
	return 0;
}
