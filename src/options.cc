#include "./options.hh"

#include <cctype>
#include <iostream>
#include <regex>

#include "global.hh"


namespace options
{
	
	bool NoArgOption::parse(std::string_view argument)
	{
		if (!argument.empty())
		{
			std::cerr << argument << '\n';
			std::cerr << "The option \"--" << getLongName() << "\" doesn't take an argument!\n";
			return false;
		}
		return parse();
	}
	
	Trilean::Trilean(const std::string_view longName, const char shortName, const getDefault_t getDefault) :
		Option(longName, shortName),
		getDefault(getDefault),
		negatedLongName("no-" + std::string(longName)),
		negated(negatedLongName, std::toupper(shortName), *this)
	{
	}
	
	bool Trilean::parse(std::string_view argument)
	{
		static const std::regex trueRegex("y(es)?|a(lways)?|t(rue)?", std::regex_constants::icase | std::regex_constants::nosubs);
		static const std::regex falseRegex("n(o)?|n(ever)?|f(alse)?", std::regex_constants::icase | std::regex_constants::nosubs);
		static const std::regex defaultRegex("d(efault)?", std::regex_constants::icase | std::regex_constants::nosubs);
		std::cmatch match;
		if (std::regex_match(argument.begin(), argument.end(), match, trueRegex) || argument.empty())
		{
			undecided = false;
			value = true;
		}
		else if (std::regex_match(argument.begin(), argument.end(), match, falseRegex))
		{
			undecided = false;
			value = false;
		}
		else if (std::regex_match(argument.begin(), argument.end(), match, defaultRegex))
		{
			undecided = true;
		}
		else
		{
			std::cerr << "Invalid value \"" << argument << "\" for the option \"--" << getLongName() << "\"!\n";
			return false;
		}
		return true;
	}
	
	
	Flag help("help", 'h');
	Flag version("version", 'v');
	
	Trilean prompt("prompt", 'p', [](){ return ::terminalInput; });
	Trilean status("status", 's', [](){ return ::terminalStderr; });
	
	std::vector<std::string_view> freeArgs;
	
	
	namespace
	{
		
		class Parser
		{
			static Option *const allOptions[];
			
			const int argc;
			const char *const *const argv;
			int i;
			
			Parser(const int argc, const char *const *const argv) : argc(argc), argv(argv) {}
			
			[[nodiscard]] bool parseShortOption(const char *&shortName, Option &option);
			[[nodiscard]] bool parseShortOption(const char *&shortName);
			[[nodiscard]] bool parseShortOptions();
			[[nodiscard]] bool parseLongOption(std::string_view argument, Option &option);
			[[nodiscard]] bool parseLongOption();
			void putRemainingtoFreeOptions();
			[[nodiscard]] bool parse();
			
		public:
			[[nodiscard]] static bool parse(const int argc, const char *const *const argv) { return Parser(argc, argv).parse(); }
		};
		
		Option *const Parser::allOptions[] = {&help, &version, &prompt, &prompt.getNegatedOption(), &status, &status.getNegatedOption()};
		
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
					std::cerr << "The option \"" << option.getLongName() << "\" requires an argument!\n";
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
					std::cerr << "The option \"" << option.getLongName() << "\" requires an argument!\n";
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
				if (option->getLongName() == longName)
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
	
	bool parse(const int argc, const char *const *const argv)
	{
		return Parser::parse(argc, argv);
	}
	
}
