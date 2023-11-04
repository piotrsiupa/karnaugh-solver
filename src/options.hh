#pragma once

#include <vector>
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
	
	class Flag : public Option
	{
		bool raised = false;
		
	public:
		using Option::Option;
		
		[[nodiscard]] bool needsArgument() const final { return false; }
		
		[[nodiscard]] bool parse(std::string_view argument) override;
		[[nodiscard]] bool isRaised() const { return raised; }
	};
	
	
	extern Flag help;
	extern Flag version;
	
	extern std::vector<std::string_view> freeOptions;
	
	
	[[nodiscard]] bool parse(const int argc, const char *const *const argv);
	
}
