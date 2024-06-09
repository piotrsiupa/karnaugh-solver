#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include "IndentedOStream.hh"
#include "options.hh"


class Names
{
public:
	using names_t = std::vector<std::string>;
	
private:
	bool useInCode;
	names_t names;
	std::string_view replacementName;
	
	void printRawNames(IndentedOStream &o, const options::FilterSpec::Filter &filter) const;
	
public:
	Names() = default;
	Names(const bool useInCode, names_t &&names, const std::string_view replacementName) : useInCode(useInCode), names(std::move(names)), replacementName(replacementName) {}
	Names(const Names &) = delete;
	Names(Names &&) = default;
	Names& operator=(const Names &) = delete;
	Names& operator=(Names &&) = default;
	
	void printPlainName(IndentedOStream &o, const std::size_t i) const { o << names[i]; }
	void printName(IndentedOStream &o, const std::size_t i) const;
	void printNames(IndentedOStream &o, const options::FilterSpec::Filter &filter = {}) const;
	void printType(IndentedOStream &o, const options::FilterSpec::Filter &filter = {}) const;
	
	[[nodiscard]] bool empty() const { return names.empty(); }
	[[nodiscard]] std::size_t size() const { return names.size(); }
	[[nodiscard]] const std::string& operator[](const std::size_t n) const { return names[n]; }
	[[nodiscard]] bool areNamesUsedInCode() const { return useInCode; }
};
