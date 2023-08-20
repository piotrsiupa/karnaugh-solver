#include "./input.hh"

#include <iostream>
#include <regex>


void trimString(std::string &string)
{
    string.erase(string.begin(), std::find_if(string.begin(), string.end(), [](unsigned char ch) { return !std::isspace(ch); }));
    string.erase(std::find_if(string.rbegin(), string.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), string.end());
}

bool loadLines(std::istream &in, lines_t &lines)
{
	lines.clear();
	while (true)
	{
		std::string line;
		std::getline(in, line);
		if (in.eof())
			return true;
		if (!in)
		{
			std::cerr << "Cannot read from stdin!\n";
			return false;
		}
		trimString(line);
		if (!line.empty() && line.front() != '#')
			lines.emplace_back(std::move(line));
	}
}

strings_t readStrings(std::string &line)
{
	strings_t strings;
	if (line == "-")
		return strings;
	static const std::regex separator("\\s*,\\s*|\\s+");
	static const std::regex_token_iterator<std::string::const_iterator> rend;
	for (std::sregex_token_iterator iter(line.cbegin(), line.cend(), separator, -1); iter != rend; ++iter)
		if (iter->length() != 0)
			strings.emplace_back(*iter);
	return strings;
}
