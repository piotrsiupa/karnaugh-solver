#include "./Input.hh"

#include <algorithm>
#include <cctype>


Minterm Input::maxMintermForMultiplying;

void Input::throwInputError(Progress *const progress)
{
	if (progress == nullptr)
		std::cerr << "Cannot read input!\n";
	else
		progress->cerr() << "Cannot read input!\n";
	throw Error("input error");
}

char Input::getChar(Progress *const progress)
{
	char c;
	istream.get(c);
	if (!istream)
	{
		if (istream.eof())
			return '\0';
		throwInputError(progress);
	}
	return c;
}

bool Input::hasNext(Progress *const progress)
{
	while (true)
	{
		if (firstChar == '\0')
		{
			return false;
		}
		else if (firstChar == '#')
		{
			while (true)
			{
				firstChar = getChar(progress);
				if (firstChar == '\0')
					return false;
				if (firstChar == '\n')
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
		if (firstChar == '\0' || firstChar == '\n')
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
		if (c == '\n' || c == '\0')
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
		if (std::isspace(c) || (c != '-' && c != '_' && std::ispunct(c)) || c == '\0')
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
		if (!std::isdigit(c))
		{
			progress.cerr() << '"' << c << "\" is not a digit!\n";
			throw Error("not a digit");
		}
		if (minterm > maxMintermForMultiplying)
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
		if (::maxMinterm - minterm < static_cast<Minterm>(c - '0'))
		{
			minterm /= 10;
			goto too_big;
		}
		minterm += c - '0';
		c = getChar(&progress);
		if (std::isspace(c) || (c != '-' && c != '_' && std::ispunct(c)) || c == '\0')
			break;
	}
	firstChar = c;
	return minterm;
}

bool Input::hasError() const
{
	return istream.bad();
}
