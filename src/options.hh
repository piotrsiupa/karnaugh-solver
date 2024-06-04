#pragma once

#include <array>
#include <cstdint>
#include <string_view>
#include <vector>

#include "Option.hh"


namespace options
{
	
	// `OutputComposer` requires this to be a bitmask in this specific order.
	enum class OutputFormat
	{
		HUMAN_LONG = 1<<4,
		HUMAN = 1<<5,
		HUMAN_SHORT = 1<<6,
		GRAPH = 1<<0,
		REDUCED_GRAPH = 1<<1,
		VERILOG = 1<<8,
		VHDL = 1<<9,
		CPP = 1<<7,
		MATHEMATICAL = 1<<2,
		GATE_COSTS = 1<<3,
	};
	
	// `OutputComposer` requires this to be in this specific order.
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
	extern Indent indent;
	extern Text name;
	extern Flag verboseGraph;
	
	extern Flag skipOptimization;
	
	extern std::vector<std::string_view> freeArgs;
	
	
	extern Option *const allOptions[12];
	
	[[nodiscard]] bool parse(const int argc, const char *const *const argv);
	
}
