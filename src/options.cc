#include "./options.hh"

#include "global.hh"


namespace options
{
	
	Flag help({"help", "full-help", "long-help"}, 'h');
	Flag helpOptions({"help-options", "truncated-help", "short-help"}, 'H');
	Flag version({"version"}, 'v');
	
	Trilean prompt({"prompt", "prompts", "hint", "hints"}, 'p', [](){
			return ::terminalInput;
		});
	Trilean status({"status", "progress", "progress-bar", "progress-bars", "stat", "stats"}, 's', [](){
#ifdef NO_DEFAULT_PROGRESS
			return false;
#else
			return ::terminalStderr;
#endif
		});
	
	bool MappedOutputFormats::supportsOperatorStyles() const
	{
		switch (getValue())
		{
		case OutputFormat::HUMAN_LONG:
		case OutputFormat::HUMAN:
		case OutputFormat::HUMAN_SHORT:
		case OutputFormat::GRAPH:
		case OutputFormat::REDUCED_GRAPH:
		case OutputFormat::MATHEMATICAL:
			return true;
		case OutputFormat::VERILOG:
		case OutputFormat::VHDL:
		case OutputFormat::CPP:
		case OutputFormat::GATE_COSTS:
			return false;
		}
		// unreachable
		return false;
	}
	
	MappedOutputFormats outputFormat({"format", "output-format", "notation", "output-notation"}, 'f', [](){ return OutputFormat::HUMAN_LONG; }, {
			{"human-long", "human(?:[-_]readable)?[-_](?:long|big)|(?:long|big)[-_]human(?:[-_]readable)?|h[-_]?(?:r[-_]?)?l|l[-_]?h(?:[-_]?r)?|full|default", OutputFormat::HUMAN_LONG},
			{"human", "human(?:[-_]readable)?(?:[-_](?:medium|middle))?|(?:(?:medium|middle)[-_])?human(?:[-_]readable)?|h(?:[-_]?r)?(?:[-_]?m)?|(?:m[-_]?)?h(?:[-_]?r)?|medium|middle|shorter", OutputFormat::HUMAN},
			{"human-short", "human(?:[-_]readable)?[-_](?:short|small)|(?:short|small)[-_]human(?:[-_]readable)?|h[-_]?(?:r[-_]?)?s|s[-_]?h(?:[-_]?r)?|short|small|tiny|minimal", OutputFormat::HUMAN_SHORT},
			{"graph", "(?:(?:f(?:u(?:ll)?)?|e(?:x(?:p(?:a(?:n(?:d(?:ed)?)?|s(?:i(?:ve)?)?)?)?)?)?|b(?:i(?:g(?:ge(?:r|st))?)?)?|l(?:a(?:r(?:ge(?:r|st)?)?)?)?)[-_ ])?(?:g(?:r(?:a(?:ph(?:s|ic(?:al)?)?)?)?)?|d(?:ot)?|vi(?:s(?:u(?:als?)?)?)?)", OutputFormat::GRAPH},
			{"reduced-graph", "(?:(?:r(?:e(?:d(?:u(?:c(?:ed)?)?)?)?)?|s(?:m(?:all(?:e(?:r|st))?)?)?|m(?:i(?:n(?:i(?:mal)?)?)?)?|c(?:o(?:m(?:p(?:act|r(?:es(?:sed)?)?))?)?)?)[-_ ])(?:g(?:r(?:a(?:ph(?:s|ic(?:al)?)?)?)?)?|d(?:ot)?|vi(?:s(?:u(?:als?)?)?)?)", OutputFormat::REDUCED_GRAPH},
			{"verilog", "verilog", OutputFormat::VERILOG},
			{"vhdl", "vhdl", OutputFormat::VHDL},
			{"cpp", "cpp|c\\+\\+|cc|hpp|h\\+\\+|hh", OutputFormat::CPP},
			{"mathematical", "math(?:ematic(?:s|al)?)?|m", OutputFormat::MATHEMATICAL},
			{"gate-costs", "(?:gates?[-_ ])?(?:costs?|scores?|stat(?:s|istics?)?|infos?)|g[-_ ]?[csi]", OutputFormat::GATE_COSTS},
		});
	Mapped<OutputOperators> outputOperators({"output-ops", "output-operators", "ops", "operators"}, 'o', [](){ return OutputOperators::FORMAL; }, {
			{"formal", "formal|f", OutputOperators::FORMAL},
			{"ascii", "ascii|a", OutputOperators::ASCII},
			{"programming", "prog(?:ramm?(?:ing)?)?|p", OutputOperators::PROGRAMMING},
			{"names", "(?:names?|words?|text)|[nwt]", OutputOperators::NAMES},
		});
	Text name({"name", "module-name", "class-name"}, 'n');
	Flag verboseGraph({"verbose-graph", "expanded-graph", "redundant-graph"}, 'G');
	
	Flag skipOptimization({"no-optimize", "no-cse", "no-optimization", "skip-optimize", "skip-cse", "skip-optimization"}, 'O');
	
	std::vector<std::string_view> freeArgs;
	
	
	const optionList_t allOptions = {&help, &helpOptions, &version, &prompt, &status, &outputFormat, &outputOperators, &name, &verboseGraph, &skipOptimization};
	
	bool parse(const int argc, const char *const *const argv)
	{
		return parse(argc, argv, allOptions, freeArgs);
	}
	
}
