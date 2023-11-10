#pragma once

#include <istream>
#include <string>
#include <vector>

#include "Progress.hh"


class Input
{
	enum class State
	{
		NOT_LOADED,
		LOADED,
		ERROR,
	};
	
	std::istream &istream;
	std::string line;
	State state = State::NOT_LOADED;
	
	void trimLine();
	void load(Progress *const progress);
	
public:
	Input(std::istream &istream) : istream(istream) {}
	
	bool hasError(Progress *const progress = nullptr);
	bool isEmpty() const { return line.empty(); }
	bool isName() const;
	std::string popLine() { state = State::NOT_LOADED; return std::move(line); }
	std::vector<std::string> popParts(Progress &progress);
};
