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
	
	void printPlainName(std::ostream &o, const std::size_t i) const { o << names[i]; }
	void printName(std::ostream &o, const std::size_t i) const;
	void printNames(std::ostream &o) const;
	void printType(std::ostream &o) const;
	
	[[nodiscard]] bool empty() const { return names.empty(); }
	[[nodiscard]] std::size_t size() const { return names.size(); }
	[[nodiscard]] bool areNamesUsedInCode() const { return useInCode; }
};
