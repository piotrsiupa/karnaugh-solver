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
	
public:
	Names() = default;
	Names(const bool useInCode, names_t &&names) : useInCode(useInCode), names(std::move(names)) {}
	Names(const Names &) = delete;
	Names& operator=(const Names &) = delete;
	Names& operator=(Names &&) = default;
	
	void printHumanName(std::ostream &o, const std::size_t i) const { o << names[i]; }
	void printVerilogName(std::ostream &o, const std::size_t i) const { if (useInCode) o << names[i]; else o << "in[" << i << ']'; }
	void printVerilogNames(std::ostream &o, const std::string_view replacementName) const;
	
	[[nodiscard]] bool isEmpty() const { return names.empty(); }
	[[nodiscard]] std::size_t getSize() const { return names.size(); }
};
