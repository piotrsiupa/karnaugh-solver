#include "./Input.hh"

#include <algorithm>
#include <iostream>
#include <regex>


void Input::trimLine()
{
    line.erase(line.begin(), std::find_if(line.begin(), line.end(), [](unsigned char ch) { return !std::isspace(ch); }));
    line.erase(std::find_if(line.rbegin(), line.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), line.end());
}

void Input::load()
{
	if (istream.eof())
	{
		line.clear();
		state = State::LOADED;
		return;
	}
	do
	{
		std::getline(istream, line);
		if (istream.eof())
			break;
		if (!istream)
		{
			std::cerr << "Cannot read from stdin!\n";
			state = State::ERROR;
			return;
		}
		trimLine();
	} while (line.empty() || line.front() == '#');
	state = State::LOADED;
}

bool Input::hasError()
{
	if (state == State::NOT_LOADED)
		load();
	return state == State::ERROR;
}


bool Input::isName() const
{
	return std::any_of(line.cbegin(), line.cend(), [](const char c){ return std::isalpha(c); });
}

std::vector<std::string> Input::popParts()
{
	state = State::NOT_LOADED;
	std::vector<std::string> parts;
	if (line == "-")
		return parts;
	static const std::regex separator("\\s*(?![-_])[[:punct:]]\\s*|\\s+");
	static const std::regex_token_iterator<std::string::const_iterator> rend;
	for (std::sregex_token_iterator iter(line.cbegin(), line.cend(), separator, -1); iter != rend; ++iter)
		if (iter->length() != 0)
			parts.emplace_back(*iter);
	return parts;
}
