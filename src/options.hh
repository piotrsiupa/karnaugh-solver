#pragma once

#include <string_view>
#include <vector>

#include "Option.hh"


namespace options
{
	
	enum class OutputFormat
	{
		HUMAN_LONG,
		HUMAN,
		HUMAN_SHORT,
		GRAPH,
		REDUCED_GRAPH,
		VERILOG,
		VHDL,
		CPP,
		MATHEMATICAL,
		GATE_COSTS,
	};
	
	enum class OutputOperators
	{
		FORMAL,
		ASCII,
		PROGRAMMING,
		NAMES,
	};
	
	extern Flag help;
	extern Flag helpOptions;
	extern Flag version;
	
	extern Trilean prompt;
	extern Trilean status;
	
	extern Mapped<OutputFormat> outputFormat;
	extern Mapped<OutputOperators> outputOperators;
	extern Text name;
	extern Flag verboseGraph;
	
	extern Flag skipOptimization;
	
	extern std::vector<std::string_view> freeArgs;
	
	
	[[nodiscard]] bool parse(const int argc, const char *const *const argv);
	
}
