#include "./options.hh"

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
			return false;
		}
		return true;
	}
	
	template<typename T, T DEFAULT_VALUE>
	void Mapped<T, DEFAULT_VALUE>::prepareRegex()
	{
		if (!regexReady)
		{
			std::string pattern;
			bool first = true;
			for (const Mapping &mapping : mappings)
			{
				if (first)
					first = false;
				else
					pattern += '|';
				pattern += '(' + std::string(mapping.first) + ')';
			}
			regex = std::regex(pattern, std::regex_constants::icase);
			regexReady = true;
		}
	}
	
	template<typename T, T DEFAULT_VALUE>
	bool Mapped<T, DEFAULT_VALUE>::parse(std::string_view argument)
	{
		prepareRegex();
		std::cmatch match;
		if (!std::regex_match(&*argument.begin(), &*argument.end(), match, regex))
		{
			std::cerr << "Invalid value \"" << argument << "\" for the option \"--" << getLongNames().front() << "\"!\n";
			return false;
		}
		for (std::size_t i = 0; i != mappings.size(); ++i)
		{
			if (match[i + 1].length() != 0)
			{
				value = mappings[i].second;
				return true;
			}
		}
		// This should be unreachable in practice.
		std::cerr << "Internal error while matching a value for the option \"--" << getLongNames().front() << "\"!\n";
		return false;
	}
	
	
	Flag help({"help", "full-help", "long-help"}, 'h');
	Flag helpOptions({"help-options", "truncated-help", "short-help"}, 'H');
	Flag version({"version"}, 'v');
	
	Trilean prompt({"prompt", "prompts", "hint", "hints"}, 'p', [](){ return ::terminalInput; });
	Trilean status({"status", "progress", "progress-bar", "progress-bars", "stat", "stats"}, 's', [](){ return ::terminalStderr; });
	
	Mapped<OutputFormat, OutputFormat::HUMAN_LONG> outputFormat({"format", "output-format", "notation", "output-notation"}, 'f', {
			{"human(?:[-_]readable)?[-_](?:long|big)|(?:long|big)[-_]human(?:[-_]readable)?|h[-_]?(?:r[-_]?)?l|l[-_]?h(?:[-_]?r)?|full|default", OutputFormat::HUMAN_LONG},
			{"human(?:[-_]readable)?(?:[-_](?:medium|middle))?|(?:(?:medium|middle)[-_])?human(?:[-_]readable)?|h(?:[-_]?r)?(?:[-_]?m)?|(?:m[-_]?)?h(?:[-_]?r)?|medium|middle|shorter", OutputFormat::HUMAN},
			{"human(?:[-_]readable)?[-_](?:short|small)|(?:short|small)[-_]human(?:[-_]readable)?|h[-_]?(?:r[-_]?)?s|s[-_]?h(?:[-_]?r)?|short|small|tiny|minimal", OutputFormat::HUMAN_SHORT},
			{"verilog", OutputFormat::VERILOG},
			{"vhdl", OutputFormat::VHDL},
			{"cpp|c\\+\\+|cc|hpp|h\\+\\+|hh", OutputFormat::CPP},
			{"math(?:ematic(?:s|al)?)?(?:[-_]formal|formal[-_]math(?:ematic(?:s|al)?)?)?|m(?:[-_]?f)?|f[-_]?m", OutputFormat::MATH_FORMAL},
			{"math(?:ematic(?:s|al)?)?[-_]prog(?:ram(?:ing)?)?|prog(?:ram(?:ming)?)?[-_]math(?:ematic(?:s|al)?)?|m[-_]?p|p[-_]?m", OutputFormat::MATH_PROG},
			{"math(?:ematic(?:s|al)?)?[-_]ascii|ascii[-_]math(?:ematic(?:s|al)?)?|m[-_]?a|a[-_]?m", OutputFormat::MATH_ASCII},
			{"math(?:ematic(?:s|al)?)?[-_](?:names?|words?|text)|(?:names?|words?|text)[-_]math(?:ematic(?:s|al)?)?|m[-_]?[nwt]|[nwt][-_]?m", OutputFormat::MATH_NAMES},
			{"(?:gates?[-_ ])?(?:costs?|scores?|stat(?:s|istics?)?|infos?)|g[-_ ]?[csi]", OutputFormat::GATE_COSTS},
		});
	Text name({"name", "module-name", "class-name"}, 'n');
	
	Flag skipOptimization({"no-optimize", "no-cse", "no-optimization", "skip-optimize", "skip-cse", "skip-optimization"}, 'O');
	
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
		
		Option *const Parser::allOptions[] = {&help, &helpOptions, &version, &prompt, &prompt.getNegatedOption(), &status, &status.getNegatedOption(), &outputFormat, &name, &skipOptimization};
		
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
	
	bool parse(const int argc, const char *const *const argv)
	{
		return Parser::parse(argc, argv);
	}
	
}
