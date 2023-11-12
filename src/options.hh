#pragma once

#include <functional>
#include <vector>
#include <string>
#include <string_view>


namespace options
{
	
	class Option
	{
		const std::vector<std::string_view> longNames;
		const char shortName;
		
		friend class OptionWithArg;
		friend class OptionWithoutArg;
		
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
		
		[[nodiscard]] bool getValue() { if (undecided) { value = getDefault(); undecided = false; } return value; }
	};
	
	
	extern Flag help;
	extern Flag version;
	
	extern Trilean prompt;
	extern Trilean status;
	
	extern Flag skipOptimization;
	
	extern std::vector<std::string_view> freeArgs;
	
	
	[[nodiscard]] bool parse(const int argc, const char *const *const argv);
	
}
