#pragma once

#include <cstddef>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>


class Names
{
public:
	using names_t = std::vector<std::string>;
	
private:
	bool useInCode;
	names_t names;
	std::string_view replacementName;
	
public:
	Names() = default;
	Names(const bool useInCode, names_t &&names, const std::string_view replacementName) : useInCode(useInCode), names(std::move(names)), replacementName(replacementName) {}
	Names(const Names &) = delete;
	Names& operator=(const Names &) = delete;
	Names& operator=(Names &&) = default;
	
	void printHumanName(std::ostream &o, const std::size_t i) const { o << names[i]; }
	void printVerilogName(std::ostream &o, const std::size_t i) const { if (useInCode) o << names[i]; else o << replacementName << '[' << i << ']'; }
	void printVerilogNames(std::ostream &o) const;
	void printVhdlName(std::ostream &o, const std::size_t i) const { if (useInCode) o << names[i]; else o << replacementName << '(' << i << ')'; }
	void printVhdlNames(std::ostream &o) const;
	void printVhdlType(std::ostream &o) const;
	void printCppRawName(std::ostream &o, const std::size_t i) const { o << names[i]; }
	void printCppName(std::ostream &o, const std::size_t i) const { o << replacementName << '[' << i << ']'; }
	void printCppType(std::ostream &o) const;
	void printMathName(std::ostream &o, const std::size_t i) const { o << names[i]; }
	void printMathNames(std::ostream &o) const;
	
	[[nodiscard]] bool isEmpty() const { return names.empty(); }
	[[nodiscard]] std::size_t getSize() const { return names.size(); }
	[[nodiscard]] bool areNamesUsedInCode() const { return useInCode; }
};
