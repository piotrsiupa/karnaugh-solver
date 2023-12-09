#pragma once

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <istream>
#include <stdexcept>
#include <string>

#include "Progress.hh"
#include "Minterm.hh"


class Input
{
	static constexpr std::uint16_t bufferCapacity = 4096;
	char buffer[bufferCapacity];
	std::uint16_t bufferSize = bufferCapacity, bufferPos = bufferCapacity;
	
	static Minterm maxMintermForMultiplying;
	
	std::istream &istream;
	char currentChar = '\n';
	
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
	
	[[nodiscard]] inline bool hasNext(Progress *const progress = nullptr);
	[[nodiscard]] inline bool hasNextInLine(Progress *const progress = nullptr);
	[[nodiscard]] inline bool isNextText() const;
	[[nodiscard]] inline bool doesNextStartWithDash() const;
	[[nodiscard]] inline std::string getLine(Progress *const progress = nullptr);
	[[nodiscard]] inline std::string getWord(Progress *const progress = nullptr);
	[[nodiscard]] inline Minterm getMinterm(Progress &progress);
	[[nodiscard]] inline bool hasError() const { return istream.bad(); }
	
	[[nodiscard]] bool isFile() const;
	[[nodiscard]] std::size_t getRemainingFileSize(); // Only for files and may of-by-one at the begining / end of file.
};

char Input::getChar(Progress *const progress)
{
	if (bufferPos == bufferSize) [[unlikely]]
	{
		istream.read(buffer, bufferCapacity);
		if (!istream) [[unlikely]]
		{
			if (!istream.eof()) [[unlikely]]
				throwInputError(progress);
			bufferSize = istream.gcount();
			if (bufferSize == 0)
				return '\0';
			istream.clear();
		}
		bufferPos = 0;
	}
	return buffer[bufferPos++];
}

bool Input::hasNext(Progress *const progress)
{
	while (true)
	{
		if (currentChar == '\0') [[unlikely]]
		{
			return false;
		}
		else if (currentChar == '#')
		{
			while (true)
			{
				currentChar = getChar(progress);
				if (currentChar == '\0') [[unlikely]]
					return false;
				else if (currentChar == '\n') [[unlikely]]
					break;
			}
		}
		else if (!std::isspace(currentChar) && (currentChar == '-' || currentChar == '_' || !std::ispunct(currentChar)))
		{
			return true;
		}
		currentChar = getChar(progress);
	}
}

bool Input::hasNextInLine(Progress *const progress)
{
	while (true)
	{
		if (currentChar == '\0' || currentChar == '\n') [[unlikely]]
			return false;
		if (!std::isspace(currentChar) && (currentChar == '-' || currentChar == '_' || !std::ispunct(currentChar)))
			return true;
		currentChar = getChar(progress);
	}
}

bool Input::isNextText() const
{
	return currentChar == '_' || std::isalpha(currentChar);
}

bool Input::doesNextStartWithDash() const
{
	return currentChar == '-';
}

std::string Input::getLine(Progress *const progress)
{
	std::string line(1, currentChar);
	while (true)
	{
		currentChar = getChar(progress);
		if (currentChar == '\n' || currentChar == '\0') [[unlikely]]
			break;
		line += currentChar;
	}
    line.erase(std::find_if(line.rbegin(), line.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), line.end());
	return line;
}

std::string Input::getWord(Progress *const progress)
{
	std::string word(1, currentChar);
	while (true)
	{
		currentChar = getChar(progress);
		if (std::isspace(currentChar) || (currentChar != '-' && currentChar != '_' && std::ispunct(currentChar)) || currentChar == '\0') [[unlikely]]
			break;
		word += currentChar;
	}
	return word;
}

Minterm Input::getMinterm(Progress &progress)
{
	Minterm minterm = 0;
	while (true)
	{
		if (!std::isdigit(currentChar)) [[unlikely]]
		{
			progress.cerr() << '"' << currentChar << "\" is not a digit!\n";
			throw Error("not a digit");
		}
		if (minterm > maxMintermForMultiplying) [[unlikely]]
		{
			too_big:
			auto cerr = progress.cerr();
			cerr << '"' << minterm;
			while (!std::isspace(currentChar) && (currentChar == '-' || currentChar == '_' || !std::ispunct(currentChar)) && currentChar != '\0')
			{
				cerr << currentChar;
				currentChar = getChar(&progress);
			}
			cerr << "\" is too big!\n";
			throw Error("number too big");
		}
		minterm *= 10;
		if (::maxMinterm - minterm < static_cast<Minterm>(currentChar - '0')) [[unlikely]]
		{
			minterm /= 10;
			goto too_big;
		}
		minterm += currentChar - '0';
		currentChar = getChar(&progress);
		if (std::isspace(currentChar) || (currentChar != '-' && currentChar != '_' && std::ispunct(currentChar)) || currentChar == '\0') [[unlikely]]
			break;
	}
	return minterm;
}
