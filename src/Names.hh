#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include "IndentedOStream.hh"


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
	Names(Names &&) = default;
	Names& operator=(const Names &) = delete;
	Names& operator=(Names &&) = default;
	
	void printPlainName(IndentedOStream &o, const std::size_t i) const { o << names[i]; }
	void printName(IndentedOStream &o, const options::OutputFormat outputFormat, const std::size_t i) const;
	void printNames(IndentedOStream &o, const options::OutputFormat outputFormat) const;
	void printType(IndentedOStream &o, const options::OutputFormat outputFormat) const;
	
	[[nodiscard]] bool empty() const { return names.empty(); }
	[[nodiscard]] std::size_t size() const { return names.size(); }
	[[nodiscard]] bool areNamesUsedInCode() const { return useInCode; }
};
