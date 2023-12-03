#pragma once

#include <stdexcept>
#include <istream>
#include <string>

#include "Progress.hh"
#include "Minterm.hh"


class Input
{
	std::istream &istream;
	char firstChar = '\n';
	static Minterm maxMintermForMultiplying;
	
	[[noreturn]] static void throwInputError(Progress *const progress);
	inline char getChar(Progress *const progress);
	
public:
	class Error : std::runtime_error
	{
	public:
		using std::runtime_error::runtime_error;
	};
	
	Input(std::istream &istream) : istream(istream) {}
	
	static void recomputeMintermSize() { maxMintermForMultiplying = ::maxMinterm / 10; }
	
	[[nodiscard]] bool hasNext(Progress *const progress = nullptr);
	[[nodiscard]] bool hasNextInLine(Progress *const progress = nullptr);
	[[nodiscard]] bool isNextText() const;
	[[nodiscard]] bool doesNextStartWithDash() const;
	[[nodiscard]] std::string getLine(Progress *const progress = nullptr);
	[[nodiscard]] std::string getWord(Progress *const progress = nullptr);
	[[nodiscard]] Minterm getMinterm(Progress &progress);
	[[nodiscard]] bool hasError() const;
};
