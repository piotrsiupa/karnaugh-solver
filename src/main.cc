#include <csignal>
#include <iostream>
#include <string>

#include "global.hh"
#include "input.hh"
#include "Karnaugh.hh"


static void trapSigint()
{
	std::signal(SIGINT, []([[maybe_unused]] const int signum){ intSignalFlag = true; });
}

names_t loadNames(std::string &line)
{
	return readStrings(line);
}

bool runKarnaugh(const names_t &names, lines_t &lines)
{
	switch (names.size())
	{
	case 2:
		return Karnaugh<2>::processMultiple(names, lines);
	case 3:
		return Karnaugh<3>::processMultiple(names, lines);
	case 4:
		return Karnaugh<4>::processMultiple(names, lines);
	case 5:
		return Karnaugh<5>::processMultiple(names, lines);
	case 6:
		return Karnaugh<6>::processMultiple(names, lines);
	case 7:
		return Karnaugh<7>::processMultiple(names, lines);
	case 8:
		return Karnaugh<8>::processMultiple(names, lines);
	case 9:
		return Karnaugh<9>::processMultiple(names, lines);
	case 10:
		return Karnaugh<10>::processMultiple(names, lines);
	case 11:
		return Karnaugh<11>::processMultiple(names, lines);
	case 12:
		return Karnaugh<12>::processMultiple(names, lines);
	case 13:
		return Karnaugh<13>::processMultiple(names, lines);
	case 14:
		return Karnaugh<14>::processMultiple(names, lines);
	case 15:
		return Karnaugh<15>::processMultiple(names, lines);
	case 16:
		return Karnaugh<16>::processMultiple(names, lines);
	default:
		std::cerr << "Unsupported number of variables!\n";
		return false;
	}
}

int main()
{
	trapSigint();
	
	lines_t lines;
	if (!loadLines(std::cin, lines))
		return 1;
		
	if (lines.empty())
	{
		std::cerr << "Input names are missing!\n";
		return 1;
	}
	const names_t names = loadNames(lines.front());
	lines.pop_front();
	
	const bool result = runKarnaugh(names, lines);
	return result ? 0 : 1;
}
