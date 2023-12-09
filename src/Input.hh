#pragma once

#include <algorithm>
#include <cctype>
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
	
	[[nodiscard]] inline bool hasNext(Progress *const progress = nullptr);
	[[nodiscard]] inline bool hasNextInLine(Progress *const progress = nullptr);
	[[nodiscard]] inline bool isNextText() const;
	[[nodiscard]] inline bool doesNextStartWithDash() const;
	[[nodiscard]] inline std::string getLine(Progress *const progress = nullptr);
	[[nodiscard]] inline std::string getWord(Progress *const progress = nullptr);
	[[nodiscard]] inline Minterm getMinterm(Progress &progress);
	[[nodiscard]] inline bool hasError() const { return istream.bad(); }
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
		if (firstChar == '\0') [[unlikely]]
		{
			return false;
		}
		else if (firstChar == '#')
		{
			while (true)
			{
				firstChar = getChar(progress);
				if (firstChar == '\0') [[unlikely]]
					return false;
				else if (firstChar == '\n') [[unlikely]]
					break;
			}
		}
		else if (!std::isspace(firstChar) && (firstChar == '-' || firstChar == '_' || !std::ispunct(firstChar)))
		{
			return true;
		}
		firstChar = getChar(progress);
	}
}

bool Input::hasNextInLine(Progress *const progress)
{
	while (true)
	{
		if (firstChar == '\0' || firstChar == '\n') [[unlikely]]
			return false;
		if (!std::isspace(firstChar) && (firstChar == '-' || firstChar == '_' || !std::ispunct(firstChar)))
			return true;
		firstChar = getChar(progress);
	}
}

bool Input::isNextText() const
{
	return firstChar == '_' || std::isalpha(firstChar);
}

bool Input::doesNextStartWithDash() const
{
	return firstChar == '-';
}

std::string Input::getLine(Progress *const progress)
{
	std::string line(1, firstChar);
	char c;
	while (true)
	{
		c = getChar(progress);
		if (c == '\n' || c == '\0') [[unlikely]]
			break;
		line += c;
	}
	firstChar = c;
    line.erase(std::find_if(line.rbegin(), line.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), line.end());
	return line;
}

std::string Input::getWord(Progress *const progress)
{
	std::string word(1, firstChar);
	char c;
	while (true)
	{
		c = getChar(progress);
		if (std::isspace(c) || (c != '-' && c != '_' && std::ispunct(c)) || c == '\0') [[unlikely]]
			break;
		word += c;
	}
	firstChar = c;
	return word;
}

Minterm Input::getMinterm(Progress &progress)
{
	Minterm minterm = 0;
	char c = firstChar;
	while (true)
	{
		if (!std::isdigit(c)) [[unlikely]]
		{
			progress.cerr() << '"' << c << "\" is not a digit!\n";
			throw Error("not a digit");
		}
		if (minterm > maxMintermForMultiplying) [[unlikely]]
		{
			too_big:
			auto cerr = progress.cerr();
			cerr << '"' << minterm;
			while (!std::isspace(c) && (c == '-' || c == '_' || !std::ispunct(c)) && c != '\0')
			{
				cerr << c;
				c = getChar(&progress);
			}
			cerr << "\" is too big!\n";
			throw Error("number too big");
		}
		minterm *= 10;
		if (::maxMinterm - minterm < static_cast<Minterm>(c - '0')) [[unlikely]]
		{
			minterm /= 10;
			goto too_big;
		}
		minterm += c - '0';
		c = getChar(&progress);
		if (std::isspace(c) || (c != '-' && c != '_' && std::ispunct(c)) || c == '\0') [[unlikely]]
			break;
	}
	firstChar = c;
	return minterm;
}
