#include "./Input.hh"

#include <algorithm>
#include <iostream>
#include <regex>
#include <utility>


void Input::trimLine()
{
	const auto commentStart = line.find_first_of('#');
	if (commentStart != std::string::npos)
		line.erase(std::next(line.begin(), commentStart), line.end());
    line.erase(line.begin(), std::find_if(line.begin(), line.end(), [](unsigned char ch) { return !std::isspace(ch); }));
    line.erase(std::find_if(line.rbegin(), line.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), line.end());
}

void Input::load(Progress *const progress)
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
			if (progress != nullptr)
				progress->cerr() << "Cannot read from stdin!\n";
			else
				std::cerr << "Cannot read from stdin!\n";
			state = State::ERROR;
			return;
		}
		trimLine();
	} while (line.empty());
	state = State::LOADED;
}

bool Input::hasError(Progress *const progress)
{
	if (state == State::NOT_LOADED)
		load(progress);
	return state == State::ERROR;
}


bool Input::isName() const
{
	return std::any_of(line.cbegin(), line.cend(), [](const char c){ return std::isalpha(c); });
}

std::vector<std::string> Input::popParts(Progress &progress)
{
	state = State::NOT_LOADED;
	std::vector<std::string> parts;
	if (line == "-")
		return parts;
	static const std::regex separator("\\s*(?![-_])[[:punct:]]\\s*|\\s+");
	static const std::sregex_token_iterator rend;
	std::sregex_token_iterator::value_type match;
	const Progress::calcStepCompletion_t calcStepCompletion = [&line = std::as_const(line), &match = std::as_const(match)](){ return static_cast<Progress::completion_t>(match.second - line.cbegin()) / static_cast<Progress::completion_t>(line.size()); };
	for (std::sregex_token_iterator iter(line.cbegin(), line.cend(), separator, -1); iter != rend; ++iter)
	{
		match = *iter;
		progress.substep(calcStepCompletion);
		if (match.length() != 0)
			parts.emplace_back(match);
	}
	return parts;
}
