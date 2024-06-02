#pragma once

#include <algorithm>
#include <charconv>
#include <cstdint>
#include <functional>
#include <limits>
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
		
		[[nodiscard]] virtual Option* getSubOption() { return nullptr; }
		
		[[nodiscard]] virtual bool needsArgument() const = 0;
		[[nodiscard]] virtual bool parse(std::string_view argument) = 0;
		[[nodiscard]] virtual bool isSet() const = 0;
		[[nodiscard]] virtual std::string compose() const = 0;  // It isn't guaranteed to work if `isSet()` is `false`.
	};
	inline std::string Option::compose() const { return "--" + std::string(getLongNames().front()); }
	
	class NoArgOption : public Option
	{
	public:
		using Option::Option;
		
		[[nodiscard]] bool needsArgument() const final { return false; }
		[[nodiscard]] bool parse(std::string_view argument) final;
		[[nodiscard]] virtual bool parse() = 0;
		[[nodiscard]] std::string compose() const final { return Option::compose(); }
	};
	
	class Flag : public NoArgOption
	{
		bool raised = false;
		
	public:
		using NoArgOption::NoArgOption;
		
		[[nodiscard]] bool parse() final { raised = true; return true; }
		[[nodiscard]] bool isSet() const final { return raised; }
		
		void raise() { raised = true; }
		[[nodiscard]] bool isRaised() const { return raised; }
		[[nodiscard]] operator bool() const { return isRaised(); }
	};
	
	class Trilean : public Option
	{
	public:
		using getDefault_t = std::function<bool()>;
		
	private:
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
			[[nodiscard]] bool isSet() const final { return false; }
		} negated;
		
	public:
		Trilean(std::vector<std::string_view> &&longNames, const char shortName, const getDefault_t getDefault);
		
		[[nodiscard]] Negated* getSubOption() final { return &negated; }
		
		[[nodiscard]] bool needsArgument() const final { return false; }
		[[nodiscard]] bool parse(std::string_view argument) final;
		[[nodiscard]] bool isSet() const final { return !undecided; }
		[[nodiscard]] std::string compose() const final { return get() ? Option::compose() : negated.compose(); }
		
		void setValue(const bool newValue) { undecided = false; value = newValue; }
		void resetValue() { undecided = true; }
		[[nodiscard]] bool get() const { return undecided ? getDefault() : value; }
		[[nodiscard]] operator bool() const { return get(); }
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
		[[nodiscard]] bool isSet() const final { return value != SIZE_MAX; }
		[[nodiscard]] std::string compose() const final { return Option::compose() + '=' + std::string(getChoiceName()); }
		
		void reset() { value = SIZE_MAX; }
		void set(const std::size_t newValue) { this->value = newValue; }
		[[nodiscard]] std::size_t get() const { return value; }
		[[nodiscard]] operator std::size_t() const { return get(); }
		[[nodiscard]] std::string_view getChoiceName(const std::size_t i) const { return variants[i].officialName; }
		[[nodiscard]] std::string_view getChoiceName() const { return isSet() ? getChoiceName(value) : "<default>"; }
	};
	
	template<typename T>
	class Mapped : public Option
	{
	public:
		using getDefault_t = std::function<T()>;
		
		struct Mapping
		{
			std::string_view officialName;
			std::string_view regex;
			T value;
		};
		using Mappings = std::vector<Mapping>;
		
	private:
		Choice choice;
		
		const getDefault_t getDefault;
		const std::vector<T> values;
		
	public:
		Mapped(std::vector<std::string_view> &&longNames, const char shortName, const getDefault_t getDefault, const Mappings &mappings) : Option(std::move(longNames), shortName), choice({getLongNames()[0]}, shortName, map_vector<Choice::Variant>(mappings, [](const Mapping &mapping) -> Choice::Variant { return {mapping.officialName, mapping.regex}; }), SIZE_MAX), getDefault(getDefault), values(map_vector<T>(mappings, [](const Mapping &mapping){ return mapping.value; })) {}
		
		[[nodiscard]] bool needsArgument() const final { return choice.needsArgument(); }
		[[nodiscard]] bool parse(std::string_view argument) final { return choice.parse(argument); }
		[[nodiscard]] bool isSet() const final { return choice.isSet(); }
		[[nodiscard]] std::string compose() const final { return choice.compose(); }
		
		void reset() { choice.reset(); }
		[[nodiscard]] T get() const { return choice.isSet() ? values[choice] : getDefault(); }
		[[nodiscard]] operator T() const { return get(); }
		[[nodiscard]] std::string_view getMappedName(const T value) const { return choice.getChoiceName(std::distance(values.cbegin(), std::find(values.cbegin(), values.cend(), value))); }
		[[nodiscard]] std::string_view getMappedName() const { return getMappedName(get()); }
	};
	
	class Text : public Option
	{
		std::optional<std::string> value;
		
	public:
		using Option::Option;
		
		[[nodiscard]] bool needsArgument() const final { return true; }
		[[nodiscard]] bool parse(std::string_view argument) final { value = argument; return true; }
		[[nodiscard]] bool isSet() const final { return static_cast<bool>(value); }
		[[nodiscard]] std::string compose() const final { return Option::compose() + '=' + *value; }
		
		void set(const std::string &newValue) { value = newValue; }
		void set(std::string &&newValue) { value = std::move(newValue); }
		[[nodiscard]] const std::optional<std::string>& get() const { return value; }
		[[nodiscard]] operator const std::optional<std::string>&() const { return get(); }
		[[nodiscard]] operator bool() const { return static_cast<bool>(value); }
		[[nodiscard]] const std::string& operator*() const { return *value; }
		[[nodiscard]] const std::optional<std::string>& operator->() const { return value; }
	};
	
	template<typename T>
	class Number : public Option
	{
		const T min, max;
		T value;
		bool isSet_ = false;
		
	public:
		Number(std::vector<std::string_view> &&longNames, const char shortName, const T min, const T max, const T initialValue) : Option(std::move(longNames), shortName), min(min), max(max), value(initialValue) {}
		
		[[nodiscard]] bool needsArgument() const final { return true; }
		[[nodiscard]] bool parse(const std::string_view argument) final;
		[[nodiscard]] bool isSet() const final { return isSet_; }
		[[nodiscard]] std::string compose() const final { return Option::compose() + '=' + std::to_string(value); }
		
		void set(const T &newValue) { value = newValue; }
		[[nodiscard]] const T& get() const { return value; }
		[[nodiscard]] operator T() const { return get(); }
	};
	
	class Indent : public Option
	{
		Number<std::uint_fast8_t> number;
		std::string indent;
		
	public:
		Indent(std::vector<std::string_view> &&longNames, const char shortName) : Option(std::move(longNames), shortName), number({}, '\0', 0, 16, 0), indent("\t") {}
		
		[[nodiscard]] bool needsArgument() const final { return true; }
		[[nodiscard]] bool parse(const std::string_view argument) final;
		[[nodiscard]] bool isSet() const final { return number.isSet(); }
		[[nodiscard]] std::string compose() const final { return Option::compose() + '=' + std::to_string(number.get()); }
		
		[[nodiscard]] const std::string& get() const { return indent; }
		[[nodiscard]] operator const std::string&() const { return get(); }
	};
	
	
	using optionList_t = std::vector<Option*>; //TODO use `std::span` in C++20
	using freeArgs_t = std::vector<std::string_view>;
	[[nodiscard]] bool parse(const int argc, const char *const *const argv, const optionList_t &allOptions, freeArgs_t &freeArgs);
	
}
