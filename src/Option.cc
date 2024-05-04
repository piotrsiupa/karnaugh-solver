#include "./Option.hh"

#include <algorithm>
#include <cctype>
#include <iostream>

#include "global.hh"


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
	
	
	namespace
	{
		
		class Parser
		{
			const int argc;
			const char *const *const argv;
			const optionList_t allOptions;
			freeArgs_t &freeArgs;
			int i;
			
			[[nodiscard]] static optionList_t expandOptions(const optionList_t &options);
			
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
		
		optionList_t Parser::expandOptions(const optionList_t &options)
		{
			optionList_t newOptionList;
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
