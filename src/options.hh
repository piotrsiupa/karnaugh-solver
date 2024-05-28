#pragma once

#include <string_view>
#include <vector>

#include "Option.hh"


namespace options
{
	
	enum class OutputFormat
	{
		HUMAN_LONG = 1<<0,
		HUMAN = 1<<1,
		HUMAN_SHORT = 1<<2,
		GRAPH = 1<<3,
		REDUCED_GRAPH = 1<<4,
		VERILOG = 1<<5,
		VHDL = 1<<6,
		CPP = 1<<7,
		MATHEMATICAL = 1<<8,
		GATE_COSTS = 1<<9,
	};
	
	enum class OutputOperators
	{
		FORMAL = 0,
		ASCII = 1,
		PROGRAMMING = 2,
		NAMES = 3,
	};
	
	extern Flag help;
	extern Flag helpOptions;
	extern Flag version;
	
	extern Trilean prompt;
	extern Trilean status;
	
	extern Trilean outputBanner;
	class MappedOutputFormats : public Mapped<OutputFormat>
	{
	public:
		using Mapped<OutputFormat>::Mapped;
		[[nodiscard]] bool supportsOperatorStyles() const;
	};
	extern MappedOutputFormats outputFormat;
	extern Mapped<OutputOperators> outputOperators;
	extern Text name;
	extern Flag verboseGraph;
	
	extern Flag skipOptimization;
	
	extern std::vector<std::string_view> freeArgs;
	
	
	extern const optionList_t allOptions;
	
	[[nodiscard]] bool parse(const int argc, const char *const *const argv);
	
}
