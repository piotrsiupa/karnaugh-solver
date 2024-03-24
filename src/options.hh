#pragma once

#include <functional>
#include <optional>
#include <regex>
#include <string>
#include <string_view>
#include <utility>
#include <vector>


namespace options
{
	
	class Option
	{
		const std::vector<std::string_view> longNames;
		const char shortName;
		
	public:
		Option(std::vector<std::string_view> &&longNames, const char shortName = '\0') : longNames(std::move(longNames)), shortName(shortName) {}
		
		[[nodiscard]] const std::vector<std::string_view>& getLongNames() const { return longNames; }
		[[nodiscard]] char getShortName() const { return shortName; }
		
		[[nodiscard]] virtual bool needsArgument() const = 0;
		[[nodiscard]] virtual bool parse(std::string_view argument) = 0;
	};
	
	class NoArgOption : public Option
	{
	public:
		using Option::Option;
		
		[[nodiscard]] bool needsArgument() const final { return false; }
		[[nodiscard]] bool parse(std::string_view argument) final;
		[[nodiscard]] virtual bool parse() = 0;
	};
	
	class Flag : public NoArgOption
	{
		bool raised = false;
		
	public:
		using NoArgOption::NoArgOption;
		
		[[nodiscard]] bool parse() final { raised = true; return true; }
		
		void raise() { raised = true; }
		[[nodiscard]] bool isRaised() const { return raised; }
	};
	
	class Trilean : public Option
	{
		using getDefault_t = std::function<bool()>;
		const getDefault_t getDefault;
		bool undecided = true, value;
		
		const std::vector<std::string> negatedLongNames;
		static std::vector<std::string> makeNegatedLongNames(const std::vector<std::string_view> &longNames);
		static std::vector<std::string_view> makeStringViews(const std::vector<std::string> &strings);
		class Negated : public NoArgOption
		{
			Trilean &trilean;
		public:
			Negated(std::vector<std::string_view> &&longNames, const char shortName, Trilean &trilean) : NoArgOption(std::move(longNames), shortName), trilean(trilean) {}
			[[nodiscard]] bool parse() final { trilean.undecided = false; trilean.value = false; return true; }
		} negated;
		
	public:
		Trilean(std::vector<std::string_view> &&longNames, const char shortName, const getDefault_t getDefault);
		
		[[nodiscard]] Option& getNegatedOption() { return negated; }
		
		[[nodiscard]] bool needsArgument() const final { return false; }
		[[nodiscard]] bool parse(std::string_view argument) final;
		
		void setValue(const bool newValue) { undecided = false; value = newValue; }
		void resetValue() { undecided = true; }
		[[nodiscard]] bool getValue() { if (undecided) { value = getDefault(); undecided = false; } return value; }
	};
	
	template<typename T, T DEFAULT_VALUE>
	class Mapped : public Option
	{
	public:
		struct Mapping
		{
			std::string_view officialName;
			std::string_view regex;
			T value;
		};
		using Mappings = std::vector<Mapping>;
		
	private:
		const Mappings mappings;
		bool regexReady = false;
		std::regex regex;
		T value = DEFAULT_VALUE;
		
		void prepareRegex();
		
	public:
		Mapped(std::vector<std::string_view> &&longNames, const char shortName, Mappings &&mappings) : Option(std::move(longNames), shortName), mappings(std::move(mappings)) {}
		
		[[nodiscard]] bool needsArgument() const final { return true; }
		[[nodiscard]] bool parse(std::string_view argument) final;
		
		void setValue(const T value) { this->value = value; }
		[[nodiscard]] T getValue() const { return value; }
	};
	
	class Text : public Option
	{
		std::optional<std::string> value;
		
	public:
		using Option::Option;
		
		[[nodiscard]] bool needsArgument() const final { return true; }
		[[nodiscard]] bool parse(std::string_view argument) final { value = argument; return true; }
		
		void setValue(const std::string &newValue) { value = newValue; }
		void setValue(std::string &&newValue) { value = std::move(newValue); }
		[[nodiscard]] const std::optional<std::string>& getValue() const { return value; }
	};
	
	
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
		MATH_FORMAL,
		MATH_ASCII,
		MATH_PROG,
		MATH_NAMES,
		GATE_COSTS,
	};
	
	extern Flag help;
	extern Flag helpOptions;
	extern Flag version;
	
	extern Trilean prompt;
	extern Trilean status;
	
	extern Mapped<OutputFormat, OutputFormat::HUMAN_LONG> outputFormat;
	extern Text name;
	extern Flag verboseGraph;
	
	extern Flag skipOptimization;
	
	extern std::vector<std::string_view> freeArgs;
	
	
	[[nodiscard]] bool parse(const int argc, const char *const *const argv);
	
}
