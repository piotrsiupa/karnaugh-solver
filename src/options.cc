#include "./options.hh"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <charconv>
#include <cstddef>
#include <iostream>
#include <limits>

#include "global.hh"
#include "utils.hh"


namespace options
{
	
	Option::Option(const std::string_view mainLongName, const std::string_view longNamesRegex, const char shortName) :
		mainLongName(mainLongName),
		longNamesRegex(longNamesRegex),
		shortName(shortName)
	{
		checkForCapturingGroups(longNamesRegex);
	}
	
#ifndef NDEBUG
	void Option::checkForCapturingGroups(const std::string_view regex)
	{
		static const std::regex capturingGroupRegex("((^|[^\\\\])(\\\\\\\\)*\\\\)?\\((\\?)?", std::regex_constants::nosubs);
		for (std::cregex_token_iterator iter(&*regex.begin(), &*regex.end(), capturingGroupRegex); iter != std::cregex_token_iterator(); ++iter)
			assert(iter->str().size() != 1);
	}
#endif
	
	bool NoArgOption::parse(std::string_view argument)
	{
		if (!argument.empty()) [[unlikely]]
		{
			std::cerr << argument << '\n';
			std::cerr << "The option \"--" << getMainLongName() << "\" doesn't take an argument!\n";
			return false;
		}
		return parse();
	}
	
	Trilean::Trilean(const std::string_view mainLongName, const std::string_view longNamesRegex, const char shortName, const getDefault_t getDefault) :
		Option(mainLongName, longNamesRegex, shortName),
		getDefault(getDefault),
		negatedMainLongName("no-" + std::string(mainLongName)),
		negatedLongNamesRegex("no-(?:" + std::string(longNamesRegex) + ')'),
		negated(negatedMainLongName, negatedLongNamesRegex, static_cast<char>(std::toupper(static_cast<unsigned char>(shortName))), *this)
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
		else [[unlikely]]
		{
			std::cerr << "Invalid value \"" << argument << "\" for the option \"--" << getMainLongName() << "\"!\n";
			std::cerr << "Allowed values are: always, never and default.\n";
			return false;
		}
		return true;
	}
	
	template<typename T>
	void Mapped<T>::prepareRegex()
	{
		if (!regexReady)
		{
			std::string pattern;
			First first;
			for (const Mapping &mapping : mappings)
			{
				if (!first)
					pattern += '|';
				pattern += '(' + std::string(mapping.regex) + ')';
			}
			regex = std::regex(pattern, std::regex_constants::icase);
			regexReady = true;
		}
	}
	
	template<typename T>
	Mapped<T>::Mapped(const std::string_view mainLongName, const std::string_view longNamesRegex, const char shortName, Mappings &&mappings, const T defaultValue) :
		Option(mainLongName, longNamesRegex, shortName),
		mappings(std::move(mappings)),
		value(defaultValue)
	{
		for (const Mapping &mapping : this->mappings)
			checkForCapturingGroups(mapping.regex);
	}
	
	template<typename T>
	bool Mapped<T>::parse(std::string_view argument)
	{
		prepareRegex();
		std::cmatch match;
		if (!std::regex_match(&*argument.begin(), &*argument.end(), match, regex)) [[unlikely]]
		{
			std::cerr << "Invalid value \"" << argument << "\" for the option \"--" << getMainLongName() << "\"!\n";
			std::cerr << "Allowed values are: ";
			First first;
			for (const Mapping &mapping : mappings)
			{
				if (!first)
					std::cerr << ", ";
				std::cerr << mapping.officialName;
			}
			std::cerr << ".\n";
			return false;
		}
		for (std::size_t i = 0; i != mappings.size(); ++i)
		{
			if (match[i + 1].length() != 0)
			{
				value = mappings[i].value;
				return true;
			}
		}
		// This should be unreachable in practice. (At least as long as the subordinate regexes don't have capturing groups.)
		[[unlikely]]
		std::cerr << "Internal error while matching a value for the option \"--" << getMainLongName() << "\"!\n";
		return false;
	}
	
	template<typename T>
	bool Number<T>::parse(const std::string_view argument)
	{
		const char *const rawBegin = argument.data(), *const rawEnd = rawBegin + argument.size();
		const auto [endOfNumber, errorCode] = std::from_chars(rawBegin, rawEnd, value);
		if (endOfNumber == rawEnd && errorCode == std::errc() && value >= min && value <= max) [[likely]]
			return true;
		if (endOfNumber != rawEnd)
		{
			std::cerr << '\"' << argument << "\" is not a number (starting at character " << (endOfNumber - rawBegin + 1) << ")!\n";
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
	
	
	// Why regex instead of one name for each option? Because it's fun. (And because I won't remember exact spelling so the program needs to figure out what I mean.)
	
	Flag help("help", "(?:(?:f(?:ull)?|l(?:ong)?)[-_ ])?h(?:elp)?|[fl]h", 'h');
	Flag helpOptions("help-options", "(?:(?:t(?:run(?:c(?:ated)?)?)?|s(?:hort)?|o(?:pt(?:ions?)?)?[-_ ]o(?:nly)?)?[-_ ]h(?:elp)?|(?:t|s|oo?)h)|(?:h(?:elp)?[-_ ](?:(?:w(?:ith)?[-_ ])?(?:o(?:nly)?|j(?:ust)?)[-_ ]o(?:pt(?:ions?)?)?|o(?:pt(?:ions?)?)?(?:[-_ ]o(?:nly)?)?)|h(?:w?[oj]o|oo))", 'H');
	Flag version("version", "v(?:er(?:s(?:ions?)?)?)?|auth(?:ors?)?", 'v');
	
	Trilean prompt("prompt", "prompts?|hints?", 'p', [](){
			return ::terminalInput;
		});
	Trilean status("status", "stat(?:s|us)?|progress(?:[-_ ]bars?)?", 's', [](){
#ifdef NO_DEFAULT_PROGRESS
			return false;
#else
			return ::terminalStderr;
#endif
		});
	
	Mapped<OutputFormat> outputFormat("format", "(?:output[-_ ])?(?:format|notation|lang(?:uage)?)", 'f', {
			{"human-long", "human(?:[-_ ]readable)?[-_ ](?:long|big)|(?:long|big)[-_ ]human(?:[-_ ]readable)?|h[-_ ]?(?:r[-_ ]?)?l|l[-_ ]?h(?:[-_ ]?r)?|full|default", OutputFormat::HUMAN_LONG},
			{"human", "human(?:[-_ ]readable)?(?:[-_ ](?:medium|middle))?|(?:(?:medium|middle)[-_ ])?human(?:[-_ ]readable)?|h(?:[-_ ]?r)?(?:[-_ ]?m)?|(?:m[-_ ]?)?h(?:[-_ ]?r)?|medium|middle|shorter", OutputFormat::HUMAN},
			{"human-short", "human(?:[-_ ]readable)?[-_ ](?:short|small)|(?:short|small)[-_ ]human(?:[-_ ]readable)?|h[-_ ]?(?:r[-_ ]?)?s|s[-_ ]?h(?:[-_ ]?r)?|short|small|tiny|minimal", OutputFormat::HUMAN_SHORT},
			{"graph", "(?:(?:f(?:u(?:ll)?)?|e(?:x(?:p(?:a(?:n(?:d(?:ed)?)?|s(?:i(?:ve)?)?)?)?)?)?|b(?:i(?:g(?:ge(?:r|st))?)?)?|l(?:a(?:r(?:ge(?:r|st)?)?)?)?)[-_ ])?(?:g(?:r(?:a(?:ph(?:s|ic(?:al)?)?)?)?)?|d(?:ot)?|vi(?:s(?:u(?:als?)?)?)?)", OutputFormat::GRAPH},
			{"reduced-graph", "(?:(?:r(?:e(?:d(?:u(?:c(?:ed)?)?)?)?)?|s(?:m(?:all(?:e(?:r|st))?)?)?|m(?:i(?:n(?:i(?:mal)?)?)?)?|c(?:o(?:m(?:p(?:act|r(?:es(?:sed)?)?))?)?)?)[-_ ])(?:g(?:r(?:a(?:ph(?:s|ic(?:al)?)?)?)?)?|d(?:ot)?|vi(?:s(?:u(?:als?)?)?)?)", OutputFormat::REDUCED_GRAPH},
			{"verilog", "verilog", OutputFormat::VERILOG},
			{"vhdl", "vhdl", OutputFormat::VHDL},
			{"cpp", "cpp|c\\+\\+|cc|hpp|h\\+\\+|hh", OutputFormat::CPP},
			{"math-formal", "math(?:ematic(?:s|al)?)?(?:[-_ ]formal|formal[-_ ]math(?:ematic(?:s|al)?)?)?|m(?:[-_ ]?f)?|f[-_ ]?m", OutputFormat::MATH_FORMAL},
			{"math-ascii", "math(?:ematic(?:s|al)?)?[-_ ]ascii|ascii[-_ ]math(?:ematic(?:s|al)?)?|m[-_ ]?a|a[-_ ]?m", OutputFormat::MATH_ASCII},
			{"math-prog", "math(?:ematic(?:s|al)?)?[-_ ]prog(?:ram(?:ing)?)?|prog(?:ram(?:ming)?)?[-_ ]math(?:ematic(?:s|al)?)?|m[-_ ]?p|p[-_ ]?m", OutputFormat::MATH_PROG},
			{"math-names", "math(?:ematic(?:s|al)?)?[-_ ](?:names?|words?|text)|(?:names?|words?|text)[-_ ]math(?:ematic(?:s|al)?)?|m[-_ ]?[nwt]|[nwt][-_ ]?m", OutputFormat::MATH_NAMES},
			{"gate-costs", "(?:gates?[-_ ])?(?:costs?|scores?|stat(?:s|istics?)?|infos?)|g[-_ ]?[csi]", OutputFormat::GATE_COSTS},
		}, OutputFormat::HUMAN_LONG);
	OptionalText name("name", "(?:(?:module|class)[-_ ])?name", 'n');
	
	Mapped<PrimeImplicantsHeuristic> primeImplicantsHeuristic("i-heuristic", "(?:p(?:rime)?[-_ ])?i(?:mpl(?:ic(?:ants?)?)?)?[-_ ]h(?:eur(?:is(?:tics?)?)?)?|p?ih", 'i', {
			{"auto", "a(?:uto)?|d(?:efault)?", PrimeImplicantsHeuristic::AUTO},
			{"brute-force", "(?:b(?:rute)?(?:[-_ ]f(?:or(?:ces?)?)?)?|bf)|s(?:low)?", PrimeImplicantsHeuristic::BRUTE_FORCE},
			{"greedy", "g(?:reedy?)?|f(?:ast)?", PrimeImplicantsHeuristic::GREEDY},
		}, PrimeImplicantsHeuristic::AUTO);
	Number<std::int_fast8_t> greedyImplicantAdjustments("greedy-i-retries", "(?:g(?:reedy?)?(?:(?:[-_ ]p(?:rime)?)?[-_ ]i(?:mpl(?:ic(?:ant)?)?)?)?|(?:(?:p(?:rime)?[-_ ])?i(?:mpl(?:ic(?:ant)?)?)?|g(?:reedy?)?)[-_ ]h(?:eur(?:is(?:t(?:ics?)?)?)?)?)[-_ ](?:(?:(?:re)?tr(?:y(?:[-_ ]count)?|ies)|(?:refine|redo|attempt|adjustment|repeat)(?:s|[-_ ]count)?|(?:pass|fix)(?:es|[-_ ]count)?)|(?:strengths?|counts?|[prtafsc]))|(?:gp?i|(?:g|p?i)?h)[prtafsc]", 'g', -1, 32, -1);
	
	Mapped<SolutionsHeuristic> solutionsHeuristics("s-heuristic", "(?:s(?:ol(?:ut(?:ions?)?)?)?(?:[-_ ](?:g(?:en(?:er(?:at(?:i(?:ng|on))?)?)?)?|m(?:erg(?:ing|es?)?)?))?|g(?:en(?:er(?:at(?:i(?:ng|on))?)?)?)?|m(?:erg(?:ing|es?)?)?)[-_ ]h(?:eur(?:is(?:tics?)?)?)?|(?:s[gm]?|[gm])h", 'z', {
			{"auto", "a(?:uto)?|d(?:efault)?", SolutionsHeuristic::AUTO},
			{"petrick", "(?:p(?:et(?:r(?:ick(?:'?s)?)?)?)?(?:[-_ ]m(?:et(?:h(?:ods?)?)?)?)?|pm?)|(?:b(?:rute)?(?:[-_ ]f(?:or(?:ces?)?)?)?|bf)|s(?:low)?", SolutionsHeuristic::PETRICK},
			{"limited", "(?:l(?:im(?:it(?:ed|s)?)?)?(?:[-_ ]p(?:et(?:r(?:ick(?:'?s)?)?)?)?(?:[-_ ]m(?:et(?:h(?:ods?)?)?)?)?)?|l(?:pm?)?)|(?:(?:p(?:et(?:r(?:ick(?:'?s)?)?)?)?(?:[-_ ]m(?:et(?:h(?:ods?)?)?)?)?[-_ ])?(?:l(?:im(?:it(?:ed|s)?)?)|(?:w(?:ith)?[-_ ])?(?:l(?:im(?:its?)?)?|m(?:ax(?:imums?)?)?))|(?:pm?)?(?:l|w?[lm]))|(?:m(?:ed(?:ium)?)?|m(?:id(?:dle)?)?)", SolutionsHeuristic::LIMITED_PETRICK},
			{"greedy", "(?:g(?:reedy?)?)|(?:f(?:ast)?)|(?:f(?:irst)?(?:[-_ ]m(?:atch)?)?|fm)", SolutionsHeuristic::GREEDY},
		}, SolutionsHeuristic::LIMITED_PETRICK);
	Number<std::size_t> solutionsLimit("solutions-limit", "(?:m(?:ax)?(?:[-_ ]s(?:ol(?:ut(?:ions?)?)?)?)?|ms?)|(?:(?:s(?:ol(?:ut(?:ions?)?)?)?[-_ ])?l(?:im(?:its?)?)?|s?l)", 'm', 0, std::numeric_limits<std::size_t>::max(), 0);
	
	Flag skipOptimization("no-optimize", "(?:no|skip)[-_ ](?:opti(?:m(?:iz(?:e|ation))?)?|cse)", 'O');
	Mapped<OptimizationHeuristic> optimizationHeuristics("cse-heuristic", "(?:(?:c(?:s|s?e)e|c(?:om(?:m(?:on)?)?)?[-_ ](?:s(?:ub(?:ex(?:pres(?:s(?:ions?)?)?)?)?)?|(?:s(?:ub)?[-_ ])?e(?:x(?:pres(?:s(?:ions?)?)?)?)?)[-_ ]e(?:lim(?:in(?:ations?)?)?|rad(?:ic(?:ations?)?)?)?)|o(?:pt(?:i(?:m(?:iz(?:ations?)?)?)?)?)?)[-_ ]h(?:eur(?:is(?:tics?)?)?)?|(?:c(?:(?:s|s?e)e)?|o)h", 'c', {
			{"brute-force", "(?:(?:f(?:ull)?[-_ ])?b(?:rute)?(?:[-_ ]f(?:or(?:ces?)?)?)?|f?bf)|slow", OptimizationHeuristic::BRUTE_FORCE},
			{"exhaustive", "(?:e(?:x(?:h(?:au(?:s(?:t(?:ive)?)?)?)?)?)?)|(?:(?:(?:a(?:l(?:m(?:ost)?)?)?|p(?:ar(?:t(?:ial)?)?)?|i(?:n(?:com(?:plete)?)?)?)[-_ ])b(?:rute)?(?:[-_ ]f(?:or(?:ces?)?)?)?|[api]bf?)", OptimizationHeuristic::EXHAUSTIVE},
			{"cursory", "c(?:u(?:r(?:so(?:ry)?)?)?)?|si(?:m(?:p(?:l(?:i(?:f(?:y|i(?:ca(?:t(?:ion)?)?)?)?)?)?)?)?)?|(?:(?:t(?:ake)?[-_ ])?a(?:ll)?(?:[-_ ]a(?:nd)?[-_ ]s(?:i(?:m(?:p(?:l(?:i(?:f(?:y)?)?)?)?)?)?)?)?|t?a(?:as)?)|(?:(?:f(?:u(?:ll)?)?|w(?:h(?:o(?:le)?)?)?)[-_ ]g(?:r(?:a(?:phs?)?)?)?|[fw]g)|(?:a(?:ll)?[-_ ](?:n(?:o(?:d(?:es?)?)?)?|e(?:d(?:g(?:es?)?)?)?)|a[ne])", OptimizationHeuristic::CURSORY},
			{"greedy", "g(?:reedy?)?", OptimizationHeuristic::GREEDY},
			{"rough", "r(?:ough)?|f(?:ast)?", OptimizationHeuristic::ROUGH},
		}, OptimizationHeuristic::BRUTE_FORCE);
	Number<std::size_t> maxRoughDepth("rough-depth", "(?:m(?:ax?)?[-_ ])?r(?:ou(?:gh)?)?(?:[-_ ]h(?:eur(?:is(?:t(?:ics?)?)?)?)?)?[-_ ](?:d(?:ep(?:ths?)?)?|(?:n(?:o(?:d(?:es?)?)?)?|s(?:ub?)?-?(?:n(?:o(?:d(?:es?)?)?)?|g(?:r(?:a(?:phs?)?)?)?|s(?:ets?)?))[-_ ]s(?:iz(?:es?)?)?)|m?rh?(?:d|(?:n|s[ngs]s)s)", 'd', 0, SIZE_MAX - 1, SIZE_MAX);
	
	std::vector<std::string_view> freeArgs;
	
	
	namespace
	{
		
		class Parser
		{
			static const std::vector<Option*> allOptions;
			static bool allOptionsRegexReady;
			static std::regex allOptionsRegex;
			
			const int argc;
			const char *const *const argv;
			int currentIndex;
			
			Parser(const int argc, const char *const *const argv) : argc(argc), argv(argv) {}
			
			void prepareAllOptionsRegex();
			
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
		
		const std::vector<Option*> Parser::allOptions = {
					&help, &helpOptions, &version,
					&prompt, &prompt.getNegatedOption(), &status, &status.getNegatedOption(),
					&outputFormat, &name,
					&primeImplicantsHeuristic, &greedyImplicantAdjustments,
					&solutionsHeuristics, &solutionsLimit,
					&skipOptimization, &optimizationHeuristics, &maxRoughDepth,
				};
		bool Parser::allOptionsRegexReady = false;
		std::regex Parser::allOptionsRegex;
		
		void Parser::prepareAllOptionsRegex()
		{
			if (!allOptionsRegexReady) [[unlikely]]
			{
				std::string pattern;
				bool first = true;
				for (const Option *const option : allOptions)
				{
					if (first) [[unlikely]]
						first = false;
					else
						pattern += '|';
					pattern += '(' + std::string(option->getLongNamesRegex()) + ')';
				}
				allOptionsRegex = std::regex(pattern, std::regex_constants::icase);
				allOptionsRegexReady = true;
			}
		}
		
		bool Parser::parseShortOption(const char *&shortName, Option &option)
		{
			if (option.needsArgument())
			{
				if (*(shortName + 1) != '\0')
				{
					if (!option.parse(++shortName)) [[unlikely]]
						return false;
					for (++shortName; *shortName != '\0'; ++shortName) {}
					--shortName;
				}
				else if (++currentIndex != argc)
				{
					if (!option.parse(argv[currentIndex])) [[unlikely]]
						return false;
				}
				else [[unlikely]]
				{
					std::cerr << "The option \"" << option.getMainLongName() << "\" requires an argument!\n";
					return false;
				}
			}
			else
			{
				if (!option.parse("")) [[unlikely]]
					return false;
			}
			return true;
		}
		
		bool Parser::parseShortOption(const char *&shortName)
		{
			for (Option *const option : allOptions)
				if (option->getShortName() == *shortName)
					return parseShortOption(shortName, *option);
			[[unlikely]]
			std::cerr << "Unknown option \"-" << *shortName << "\"!\n";
			return false;
		}
		
		bool Parser::parseShortOptions()
		{
			for (const char *shortName = argv[currentIndex] + 1; *shortName != '\0'; ++shortName)
				if (!parseShortOption(shortName)) [[unlikely]]
					return false;
			return true;
		}
		
		bool Parser::parseLongOption(std::string_view argument, Option &option)
		{
			if (option.needsArgument() && argument.empty())
			{
				if (++currentIndex == argc) [[unlikely]]
				{
					std::cerr << "The option \"" << option.getMainLongName() << "\" requires an argument!\n";
					return false;
				}
				argument = argv[currentIndex];
			}
			if (!option.parse(argument)) [[unlikely]]
				return false;
			return true;
		}
		
		bool Parser::parseLongOption()
		{
			std::string_view longName = argv[currentIndex] + 2, argument = "";
			if (const std::string_view::size_type pos = longName.find('='); pos != std::string_view::npos)
			{
				argument = longName.substr(pos + 1);
				longName = longName.substr(0, pos);
			}
			prepareAllOptionsRegex();
			std::cmatch match;
			if (!std::regex_match(&*longName.begin(), &*longName.end(), match, allOptionsRegex)) [[unlikely]]
			{
				std::cerr << "Unknown option \"--" << longName << "\"!\n";
				return false;
			}
			for (std::size_t i = 0; i != allOptions.size(); ++i)
				if (match[i + 1].length() != 0)
					return parseLongOption(argument, *allOptions[i]);
			// This should be unreachable in practice. (At least as long as the subordinate regexes don't have capturing groups.)
			[[unlikely]]
			std::cerr << "Critical failure of the options parsing code!\n";
			return false;
		}
		
		void Parser::putRemainingtoFreeOptions()
		{
			for (; currentIndex != argc; ++currentIndex)
				freeArgs.emplace_back(argv[currentIndex]);
		}
		
		bool Parser::parse()
		{
			for (currentIndex = 1; currentIndex != argc; ++currentIndex)
			{
				if (argv[currentIndex][0] != '-')
				{
					freeArgs.emplace_back(argv[currentIndex]);
				}
				else
				{
					if (argv[currentIndex][1] == '\0')
					{
						freeArgs.emplace_back(argv[currentIndex]);
					}
					else if (argv[currentIndex][1] != '-')
					{
						if (!parseShortOptions()) [[unlikely]]
							return false;
					}
					else
					{
						if (argv[currentIndex][2] == '\0')
						{
							++currentIndex;
							break;
						}
						else
						{
							if (!parseLongOption()) [[unlikely]]
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
