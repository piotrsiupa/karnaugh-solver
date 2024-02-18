#pragma once

#include <cstdint>
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
		const std::string_view mainLongName;
		const std::string_view longNamesRegex;
		const char shortName;
		
	public:
		Option(const std::string_view mainLongName, const std::string_view longNamesRegex, const char shortName = '\0') : mainLongName(mainLongName), longNamesRegex(longNamesRegex), shortName(shortName) {}
		
		[[nodiscard]] const std::string_view& getMainLongName() const { return mainLongName; }
		[[nodiscard]] const std::string_view& getLongNamesRegex() const { return longNamesRegex; }
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
		
		const std::string negatedMainLongName;
		const std::string negatedLongNamesRegex;
		class Negated : public NoArgOption
		{
			Trilean &trilean;
		public:
			Negated(const std::string_view mainLongName, const std::string_view longNamesRegex, const char shortName, Trilean &trilean) : NoArgOption(mainLongName, longNamesRegex, shortName), trilean(trilean) {}
			[[nodiscard]] bool parse() final { trilean.undecided = false; trilean.value = false; return true; }
		} negated;
		
	public:
		Trilean(const std::string_view mainLongName, const std::string_view longNamesRegex, const char shortName, const getDefault_t getDefault);
		
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
		using Mapping = std::pair<std::string_view, T>;
		using Mappings = std::vector<Mapping>;
		
	private:
		const Mappings mappings;
		bool regexReady = false;
		std::regex regex;
		T value = DEFAULT_VALUE;
		
		void prepareRegex();
		
	public:
		Mapped(const std::string_view mainLongName, const std::string_view longNamesRegex, const char shortName, Mappings &&mappings) : Option(mainLongName, longNamesRegex, shortName), mappings(std::move(mappings)) {}
		
		[[nodiscard]] bool needsArgument() const final { return true; }
		[[nodiscard]] bool parse(std::string_view argument) final;
		
		void setValue(const T value) { this->value = value; }
		[[nodiscard]] T getValue() const { return value; }
	};
	
	class OptionalText : public Option
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
	
	template<typename T>
	class Number : public Option
	{
		const T min, max;
		T value;
		
	public:
		Number(const std::string_view mainLongName, const std::string_view longNamesRegex, const char shortName, const T min, const T max, const T initialValue) : Option(mainLongName, longNamesRegex, shortName), min(min), max(max), value(initialValue) {}
		
		[[nodiscard]] bool needsArgument() const final { return true; }
		[[nodiscard]] bool parse(std::string_view argument) final;
		
		void setValue(const T &newValue) { value = newValue; }
		void setValue(T &&newValue) { value = std::move(newValue); }
		[[nodiscard]] const T& getValue() const { return value; }
	};
	
	
	enum class OutputFormat
	{
		HUMAN_LONG,
		HUMAN,
		HUMAN_SHORT,
		VERILOG,
		VHDL,
		CPP,
		MATH_FORMAL,
		MATH_PROG,
		MATH_ASCII,
		MATH_NAMES,
		GATE_COSTS,
	};
	
	enum class PrimeImplicantsHeuristic
	{
		BRUTE_FORCE,
		AUTO,
		GREEDY,
	};
	
	extern Flag help;
	extern Flag version;
	
	extern Trilean prompt;
	extern Trilean status;
	
	extern Flag skipOptimization;
	extern Mapped<OutputFormat, OutputFormat::HUMAN_LONG> outputFormat;
	extern OptionalText name;
	
	extern std::vector<std::string_view> freeArgs;
	
	extern Mapped<PrimeImplicantsHeuristic, PrimeImplicantsHeuristic::AUTO> primeImplicantsHeuristic;
	extern Number<std::int_fast8_t> greedyImplicantAdjustments;
	
	
	[[nodiscard]] bool parse(const int argc, const char *const *const argv);
	
}
