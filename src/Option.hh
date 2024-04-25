#pragma once

#include <functional>
#include <optional>
#include <regex>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "utils.hh"


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
	
	class Choice : public Option
	{
	public:
		struct Variant
		{
			std::string_view officialName;
			std::string_view regex;
		};
		using Variants = std::vector<Variant>;
		
	private:
		const Variants variants;
		bool regexReady = false;
		std::regex regex;
		std::size_t value;
		
		void prepareRegex();
		
	public:
		Choice(std::vector<std::string_view> &&longNames, const char shortName, Variants &&variants, const std::size_t defaultValue) : Option(std::move(longNames), shortName), variants(std::move(variants)), value(defaultValue) {}
		
		[[nodiscard]] bool needsArgument() const final { return true; }
		[[nodiscard]] bool parse(std::string_view argument) final;
		
		void setValue(const std::size_t newValue) { this->value = newValue; }
		[[nodiscard]] std::size_t getValue() const { return value; }
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
		Choice choice;
		const std::vector<T> values;
		
	public:
		Mapped(std::vector<std::string_view> &&longNames, const char shortName, const Mappings &mappings) : Option(std::move(longNames), shortName), choice({getLongNames()[0]}, shortName, map_vector<Choice::Variant>(mappings, [](const Mapping &mapping) -> Choice::Variant { return {mapping.officialName, mapping.regex}; }), SIZE_MAX), values(map_vector<T>(mappings, [](const Mapping &mapping){ return mapping.value; })) {}
		
		[[nodiscard]] bool needsArgument() const final { return choice.needsArgument(); }
		[[nodiscard]] bool parse(std::string_view argument) final { return choice.parse(argument); }
		
		//void setValue(const T value) { this->value = value; } //TODO set variable storing default value and then `choice.setValue(SIZE_MAX);` (waiting for the merge of the branch in which the default value is no longet a template parameter)
		[[nodiscard]] T getValue() const { const std::size_t i = choice.getValue(); return i == SIZE_MAX ? DEFAULT_VALUE : values[i]; }
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
	
	
	using optionList_t = std::vector<Option*>; //TODO use `std::span` in C++20
	using freeArgs_t = std::vector<std::string_view>;
	[[nodiscard]] bool parse(const int argc, const char *const *const argv, const optionList_t &allOptions, freeArgs_t &freeArgs);
	
}
