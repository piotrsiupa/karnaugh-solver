#pragma once

#include <istream>
#include <string>
#include <vector>


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
	void load();
	
public:
	Input(std::istream &istream) : istream(istream) {}
	
	bool hasError();
	bool isEmpty() const { return line.empty(); }
	bool isName() const;
	std::string popLine() { state = State::NOT_LOADED; return std::move(line); }
	std::vector<std::string> popParts();
};
