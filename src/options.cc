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
	
	Mapped<OutputFormat> outputFormat({"format", "output-format", "notation", "output-notation"}, 'f', [](){ return OutputFormat::HUMAN_LONG; }, {
			{"human-long", "human(?:[-_]readable)?[-_](?:long|big)|(?:long|big)[-_]human(?:[-_]readable)?|h[-_]?(?:r[-_]?)?l|l[-_]?h(?:[-_]?r)?|full|default", OutputFormat::HUMAN_LONG},
			{"human", "human(?:[-_]readable)?(?:[-_](?:medium|middle))?|(?:(?:medium|middle)[-_])?human(?:[-_]readable)?|h(?:[-_]?r)?(?:[-_]?m)?|(?:m[-_]?)?h(?:[-_]?r)?|medium|middle|shorter", OutputFormat::HUMAN},
			{"human-short", "human(?:[-_]readable)?[-_](?:short|small)|(?:short|small)[-_]human(?:[-_]readable)?|h[-_]?(?:r[-_]?)?s|s[-_]?h(?:[-_]?r)?|short|small|tiny|minimal", OutputFormat::HUMAN_SHORT},
			{"graph", "(?:(?:f(?:u(?:ll)?)?|e(?:x(?:p(?:a(?:n(?:d(?:ed)?)?|s(?:i(?:ve)?)?)?)?)?)?|b(?:i(?:g(?:ge(?:r|st))?)?)?|l(?:a(?:r(?:ge(?:r|st)?)?)?)?)[-_ ])?(?:g(?:r(?:a(?:ph(?:s|ic(?:al)?)?)?)?)?|d(?:ot)?|vi(?:s(?:u(?:als?)?)?)?)", OutputFormat::GRAPH},
			{"reduced-graph", "(?:(?:r(?:e(?:d(?:u(?:c(?:ed)?)?)?)?)?|s(?:m(?:all(?:e(?:r|st))?)?)?|m(?:i(?:n(?:i(?:mal)?)?)?)?|c(?:o(?:m(?:p(?:act|r(?:es(?:sed)?)?))?)?)?)[-_ ])(?:g(?:r(?:a(?:ph(?:s|ic(?:al)?)?)?)?)?|d(?:ot)?|vi(?:s(?:u(?:als?)?)?)?)", OutputFormat::REDUCED_GRAPH},
			{"verilog", "verilog", OutputFormat::VERILOG},
			{"vhdl", "vhdl", OutputFormat::VHDL},
			{"cpp", "cpp|c\\+\\+|cc|hpp|h\\+\\+|hh", OutputFormat::CPP},
			{"math-formal", "math(?:ematic(?:s|al)?)?(?:[-_]formal|formal[-_]math(?:ematic(?:s|al)?)?)?|m(?:[-_]?f)?|f[-_]?m", OutputFormat::MATH_FORMAL},
			{"math-ascii", "math(?:ematic(?:s|al)?)?[-_]ascii|ascii[-_]math(?:ematic(?:s|al)?)?|m[-_]?a|a[-_]?m", OutputFormat::MATH_ASCII},
			{"math-prog", "math(?:ematic(?:s|al)?)?[-_]prog(?:ram(?:ing)?)?|prog(?:ram(?:ming)?)?[-_]math(?:ematic(?:s|al)?)?|m[-_]?p|p[-_]?m", OutputFormat::MATH_PROG},
			{"math-names", "math(?:ematic(?:s|al)?)?[-_](?:names?|words?|text)|(?:names?|words?|text)[-_]math(?:ematic(?:s|al)?)?|m[-_]?[nwt]|[nwt][-_]?m", OutputFormat::MATH_NAMES},
			{"gate-costs", "(?:gates?[-_ ])?(?:costs?|scores?|stat(?:s|istics?)?|infos?)|g[-_ ]?[csi]", OutputFormat::GATE_COSTS},
		});
	Text name({"name", "module-name", "class-name"}, 'n');
	Flag verboseGraph({"verbose-graph", "expanded-graph", "redundant-graph"}, 'G');
	
	Flag skipOptimization({"no-optimize", "no-cse", "no-optimization", "skip-optimize", "skip-cse", "skip-optimization"}, 'O');
	
	std::vector<std::string_view> freeArgs;
	
	
	static const optionList_t allOptions = {&help, &helpOptions, &version, &prompt, &prompt.getNegatedOption(), &status, &status.getNegatedOption(), &outputFormat, &name, &verboseGraph, &skipOptimization};
	
	bool parse(const int argc, const char *const *const argv)
	{
		return parse(argc, argv, allOptions, freeArgs);
	}
	
}
