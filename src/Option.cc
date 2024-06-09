#include "./Option.hh"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <ranges>

#include "global.hh"
#include "Names.hh"


namespace options
{
	
	bool NoArgOption::parse(std::string_view argument)
	{
		if (!argument.empty())
		{
			std::cerr << argument << '\n';
			std::cerr << "The option \"--" << getLongNames().front() << "\" doesn't take an argument!\n";
			return false;
		}
		return parse();
	}
	
	
	std::vector<std::string> Trilean::makeNegatedLongNames(const std::vector<std::string_view> &longNames)
	{
		std::vector<std::string> negatedLongNames;
		negatedLongNames.reserve(longNames.size());
		for (const std::string_view &longName : longNames)
			negatedLongNames.emplace_back("no-" + std::string(longName));
		return negatedLongNames;
	}
	
	std::vector<std::string_view> Trilean::makeStringViews(const std::vector<std::string> &strings)
	{
		std::vector<std::string_view> stringViews;
		stringViews.reserve(strings.size());
		for (const std::string &string : strings)
			stringViews.emplace_back(string);
		return stringViews;
	}
	
	Trilean::Trilean(std::vector<std::string_view> &&longNames, const char shortName, const getDefault_t getDefault) :
		Option(std::move(longNames), shortName),
		getDefault(getDefault),
		negatedLongNames(makeNegatedLongNames(getLongNames())),
		negated(makeStringViews(negatedLongNames), static_cast<char>(std::toupper(static_cast<unsigned char>(shortName))), *this)
	{
	}
	
	bool Trilean::parse(std::string_view argument)
	{
		static const std::regex trueRegex("y(es)?|a(lways)?|t(rue)?|1", std::regex_constants::icase | std::regex_constants::nosubs);
		static const std::regex falseRegex("n(o)?|n(ever)?|f(alse)?|0", std::regex_constants::icase | std::regex_constants::nosubs);
		static const std::regex defaultRegex("d(efault)?|auto", std::regex_constants::icase | std::regex_constants::nosubs);
		std::cmatch match;
		if (std::regex_match(&*argument.begin(), &*argument.end(), match, trueRegex) || argument.empty())
		{
			undecided = false;
			value = true;
		}
		else if (std::regex_match(&*argument.begin(), &*argument.end(), match, falseRegex))
		{
			undecided = false;
			value = false;
		}
		else if (std::regex_match(&*argument.begin(), &*argument.end(), match, defaultRegex))
		{
			undecided = true;
		}
		else
		{
			std::cerr << "Invalid value \"" << argument << "\" for the option \"--" << getLongNames().front() << "\"!\n";
			std::cerr << "Allowed values are: always, never and default.\n";
			return false;
		}
		return true;
	}
	
	void Choice::prepareRegex()
	{
		if (!regexReady)
		{
			std::string pattern;
			First first;
			for (const Variant &variant : variants)
			{
				if (!first)
					pattern += '|';
				pattern += '(' + std::string(variant.regex) + ')';
			}
			regex = std::regex(pattern, std::regex_constants::icase);
			regexReady = true;
		}
	}
	
	
	bool Choice::parse(std::string_view argument)
	{
		prepareRegex();
		std::cmatch match;
		if (!std::regex_match(&*argument.begin(), &*argument.end(), match, regex))
		{
			std::cerr << "Invalid value \"" << argument << "\" for the option \"--" << getLongNames().front() << "\"!\n";
			std::cerr << "Allowed values are: ";
			First first;
			for (const Variant &variant : variants)
			{
				if (!first)
					std::cerr << ", ";
				std::cerr << variant.officialName;
			}
			std::cerr << ".\n";
			return false;
		}
		for (std::size_t i = 0; i != variants.size(); ++i)
		{
			if (match[i + 1].length() != 0)
			{
				value = i;
				return true;
			}
		}
		// This should be unreachable in practice.
		std::cerr << "Internal error while matching a value for the option \"--" << getLongNames().front() << "\"!\n";
		return false;
	}
	
	
	template<typename T>
	bool Number<T>::parse(const std::string_view argument)
	{
		const auto [endOfNumber, errorCode] = std::from_chars(argument.cbegin(), argument.cend(), value);
		if (endOfNumber == argument.cend() && errorCode == std::errc() && value >= min && value <= max) [[likely]]
		{
			isSet_ = true;
			return true;
		}
		if (endOfNumber != argument.cend())
		{
			std::cerr << '\"' << argument << "\" is not a number (starting at character " << (endOfNumber - argument.cbegin() + 1) << ")!\n";
		}
		else
		{
			if constexpr (std::numeric_limits<T>::is_signed)
				std::cerr << '\"' << argument << "\" is out of range (" << static_cast<std::intmax_t>(min) << ".." << static_cast<std::intmax_t>(max) << ")!\n";
			else
				std::cerr << '\"' << argument << "\" is out of range (" << static_cast<std::uintmax_t>(min) << ".." << static_cast<std::uintmax_t>(max) << ")!\n";
		}
		return false;
	}
	
	template class Number<std::uint_fast8_t>;
	
	
	bool Indent::parse(std::string_view argument)
	{
		if (!argument.empty() && (argument.front() == '+' || argument.front() == '-'))
		{
			indentEmpty = argument.front() == '+';
			argument.remove_prefix(1);
		}
		if (!number.parse(argument))
			return false;
		if (number != 0)
			indent = std::string(number, ' ');
		return true;
	}
	
	
	FilterSpec::funcRef_t FilterSpec::stringToFuncRef(const std::string_view string)
	{
		const char *const begin = string.data(), *const end = begin + string.size();
		std::size_t number;
		const auto [endOfNumber, errorCode] = std::from_chars(begin, end, number);
		if (endOfNumber != end || errorCode != std::errc())
			return std::string(string);
		else
			return number;
	}
	
	bool FilterSpec::parse(const std::string_view argument)
	{
		for (const auto x : std::ranges::split_view(argument, ','))
		{
			static constexpr std::string_view rangeMarker = "..";
			const std::string_view range(&*x.begin(), std::ranges::distance(x));
			std::string_view::size_type rangeMarkerPos = range.find(rangeMarker);
			std::string_view begin, end;
			if (rangeMarkerPos == std::string_view::npos)
			{
				begin = end = stripString(range);
			}
			else if (range.find(rangeMarker, rangeMarkerPos + 1) == std::string_view::npos)
			{
				begin = stripString(range.substr(0, rangeMarkerPos));
				rangeMarkerPos += rangeMarker.size();
				end = stripString(range.substr(rangeMarkerPos, range.size() - rangeMarkerPos));
			}
			else
			{
				std::cerr << "Range decriptor can contain only one \"..\"!\n";
				return false;
			}
			functionRefs.emplace_back(stringToFuncRef(begin), stringToFuncRef(end));
		}
		return true;
	}
	
	std::string FilterSpec::compose() const
	{
		std::string result = Option::compose() + '=';
		First first;
		for (const funcRefRange_t &range : functionRefs)
		{
			if (!first)
				result += ',';
			result += std::holds_alternative<std::size_t>(range.first) ? std::to_string(std::get<std::size_t>(range.first)) : std::get<std::string>(range.first);
			if (range.first != range.last)
			{
				result += "..";
				result += std::holds_alternative<std::size_t>(range.last) ? std::to_string(std::get<std::size_t>(range.last)) : std::get<std::string>(range.last);
			}
		}
		return result;
	}
	
	bool FilterSpec::prepare(const Names &functionNames)
	{
		const auto prepareRef = [&functionNames](const funcRef_t &ref)->std::size_t{
				if (std::holds_alternative<std::size_t>(ref))
					return std::get<std::size_t>(ref);
				for (std::size_t i = 0; i != functionNames.size(); ++i)
					if (std::get<std::string>(ref) == functionNames[i])
						return i;
				return SIZE_MAX;
			};
		Filter::ranges_t preparedRanges;
		for (const funcRefRange_t &range : functionRefs)
		{
			preparedRanges.emplace_back(prepareRef(range.first), prepareRef(range.last));
			if (preparedRanges.back().first >= functionNames.size() || preparedRanges.back().last >= functionNames.size()) [[unlikely]]
			{
				std::cerr << "Specified function was not found!\n";
				return false;
			}
			if (preparedRanges.back().first > preparedRanges.back().last) [[unlikely]]
			{
				std::cerr << "The first element of a range is bigger than the last one!\n";
				return false;
			}
		}
		limit = std::move(preparedRanges);
		return true;
	}
	
	bool FilterSpec::Filter::operator[](const std::size_t n) const
	{
		if (ranges.empty())
			return true;
		for (const range_t &range : ranges)
			if (range.first <= n && range.last >= n)
				return true;
		return false;
	}
	
	std::size_t FilterSpec::Filter::count(const std::size_t totalCount) const
	{
		if (ranges.empty())
			return totalCount;
		std::size_t count = 0;
		for (const range_t &range : ranges)
			count += range.last - range.first + 1;
		return count;
	}
	
	
	namespace
	{
		
		class Parser
		{
			const int argc;
			const char *const *const argv;
			const std::vector<Option*> allOptions;
			freeArgs_t &freeArgs;
			int i;
			
			[[nodiscard]] static std::vector<Option*> expandOptions(const optionList_t &options);
			
			Parser(const int argc, const char *const *const argv, const optionList_t &allOptions, freeArgs_t &freeArgs) : argc(argc), argv(argv), allOptions(expandOptions(allOptions)), freeArgs(freeArgs) {}
			
			[[nodiscard]] bool parseShortOption(const char *&shortName, Option &option);
			[[nodiscard]] bool parseShortOption(const char *&shortName);
			[[nodiscard]] bool parseShortOptions();
			[[nodiscard]] bool parseLongOption(std::string_view argument, Option &option);
			[[nodiscard]] bool parseLongOption();
			void putRemainingtoFreeOptions();
			[[nodiscard]] bool parse();
			
		public:
			[[nodiscard]] static bool parse(const int argc, const char *const *const argv, const optionList_t &allOptions, freeArgs_t &freeArgs) { return Parser(argc, argv, allOptions, freeArgs).parse(); }
		};
		
		std::vector<Option*> Parser::expandOptions(const optionList_t &options)
		{
			std::vector<Option*> newOptionList;
			for (Option *option : options)
			{
				while (option != nullptr)
				{
					newOptionList.push_back(option);
					option = option->getSubOption();
				}
			}
			return newOptionList;
		}
		
		bool Parser::parseShortOption(const char *&shortName, Option &option)
		{
			if (option.needsArgument())
			{
				if (*(shortName + 1) != '\0')
				{
					if (!option.parse(++shortName))
						return false;
					for (++shortName; *shortName != '\0'; ++shortName) {}
					--shortName;
				}
				else if (++i != argc)
				{
					if (!option.parse(argv[i]))
						return false;
				}
				else
				{
					std::cerr << "The option \"" << option.getLongNames().front() << "\" requires an argument!\n";
					return false;
				}
			}
			else
			{
				if (!option.parse(""))
					return false;
			}
			return true;
		}
		
		bool Parser::parseShortOption(const char *&shortName)
		{
			for (Option *const option : allOptions)
				if (option->getShortName() == *shortName)
					return parseShortOption(shortName, *option);
			std::cerr << "Unknown option \"-" << *shortName << "\"!\n";
			return false;
		}
		
		bool Parser::parseShortOptions()
		{
			for (const char *shortName = argv[i] + 1; *shortName != '\0'; ++shortName)
				if (!parseShortOption(shortName))
					return false;
			return true;
		}
		
		bool Parser::parseLongOption(std::string_view argument, Option &option)
		{
			if (option.needsArgument() && argument.empty())
			{
				if (++i == argc)
				{
					std::cerr << "The option \"" << option.getLongNames().front() << "\" requires an argument!\n";
					return false;
				}
				argument = argv[i];
			}
			if (!option.parse(argument))
				return false;
			return true;
		}
		
		bool Parser::parseLongOption()
		{
			std::string_view longName = argv[i] + 2, argument = "";
			if (const std::string_view::size_type pos = longName.find('='); pos != std::string_view::npos)
			{
				argument = longName.substr(pos + 1);
				longName = longName.substr(0, pos);
			}
			for (Option *const option : allOptions)
				if (std::find(option->getLongNames().cbegin(), option->getLongNames().cend(), longName) != option->getLongNames().cend())
					return parseLongOption(argument, *option);
			std::cerr << "Unknown option \"--" << longName << "\"!\n";
			return false;
		}
		
		void Parser::putRemainingtoFreeOptions()
		{
			for (; i != argc; ++i)
				freeArgs.emplace_back(argv[i]);
		}
		
		bool Parser::parse()
		{
			for (i = 1; i != argc; ++i)
			{
				if (argv[i][0] != '-')
				{
					freeArgs.emplace_back(argv[i]);
				}
				else
				{
					if (argv[i][1] == '\0')
					{
						freeArgs.emplace_back(argv[i]);
					}
					else if (argv[i][1] != '-')
					{
						if (!parseShortOptions())
							return false;
					}
					else
					{
						if (argv[i][2] == '\0')
						{
							++i;
							break;
						}
						else
						{
							if (!parseLongOption())
								return false;
						}
					}
				}
			}
			putRemainingtoFreeOptions();
			return true;
		}
		
	}
	
	bool parse(const int argc, const char *const *const argv, const optionList_t &allOptions, freeArgs_t &freeArgs)
	{
		return Parser::parse(argc, argv, allOptions, freeArgs);
	}
	
}
