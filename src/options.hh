#pragma once

#include <functional>
#include <vector>
#include <string>
#include <string_view>


namespace options
{
	
	class Option
	{
		const std::string_view longName;
		const char shortName;
		
		friend class OptionWithArg;
		friend class OptionWithoutArg;
		
	public:
		Option(const std::string_view longName, const char shortName = '\0') : longName(longName), shortName(shortName) {}
		
		[[nodiscard]] const std::string_view getLongName() const { return longName; }
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
		
		const std::string negatedLongName;
		class Negated : public NoArgOption
		{
			Trilean &trilean;
		public:
			Negated(const std::string_view longName, const char shortName, Trilean &trilean) : NoArgOption(longName, shortName), trilean(trilean) {}
			[[nodiscard]] bool parse() final { trilean.undecided = false; trilean.value = false; return true; }
		} negated;
		
	public:
		Trilean(const std::string_view longName, const char shortName, const getDefault_t getDefault);
		
		[[nodiscard]] Option& getNegatedOption() { return negated; }
		
		[[nodiscard]] bool needsArgument() const final { return false; }
		[[nodiscard]] bool parse(std::string_view argument) final;
		
		[[nodiscard]] bool getValue() { if (undecided) { value = getDefault(); undecided = false; } return value; }
	};
	
	
	extern Flag help;
	extern Flag version;
	
	extern Trilean prompt;
	extern Trilean status;
	
	extern std::vector<std::string_view> freeArgs;
	
	
	[[nodiscard]] bool parse(const int argc, const char *const *const argv);
	
}
