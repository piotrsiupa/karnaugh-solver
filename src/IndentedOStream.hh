#pragma once

#include <cassert>
#include <cstddef>
#include <ostream>
#include <utility>
#include <type_traits>

#include "options.hh"


class IndentedOStream
{
	std::ostream *o;
	std::size_t indentSize = 0;
	bool newLine = true;
	bool sanitization = false;
	
	void insertIndent();
	void printText(const std::string_view text);
	
public:
	IndentedOStream() : o(nullptr) {}
	IndentedOStream(std::ostream &o) : o(&o) {}
	
	void setStream(std::ostream &newO) { o = &newO; }
	
	template<typename T>
	IndentedOStream& operator<<(T x);
	
	void indent(const std::ptrdiff_t x = 1) { assert(x >= 0 ? SIZE_MAX - indentSize >= static_cast<size_t>(x) : indentSize >= static_cast<size_t>(-x)); indentSize += x; }
	void setIndent(const std::size_t x) { indentSize = x; }
	class IndentGuard
	{
		IndentedOStream &stream;
		const std::size_t startSize;
		IndentGuard(IndentedOStream &stream, const std::ptrdiff_t x) : stream(stream), startSize(stream.indentSize) { stream.indent(x); }
		friend class IndentedOStream;
	public:
		IndentGuard(const IndentGuard &) = delete;
		IndentGuard& operator=(const IndentGuard &) = delete;
		~IndentGuard() { reset(); }
		void reset() const { stream.setIndent(startSize); }
	};
	[[nodiscard]] IndentGuard startIndent(const std::ptrdiff_t x = 1) { return {*this, x}; }
	
	void sanitize(const bool x) { sanitization = x; }
	class SanitizeGuard
	{
		IndentedOStream &stream;
		const bool startState;
		SanitizeGuard(IndentedOStream &stream, const bool x) : stream(stream), startState(stream.sanitization) { stream.sanitize(x); }
		friend class IndentedOStream;
	public:
		SanitizeGuard(const SanitizeGuard &) = delete;
		SanitizeGuard& operator=(const SanitizeGuard &) = delete;
		~SanitizeGuard() { reset(); }
		void reset() const { stream.sanitize(startState); }
	};
	[[nodiscard]] SanitizeGuard startSanitize(const bool x = true) { return {*this, x}; }
};


template<typename T>
IndentedOStream& IndentedOStream::operator<<(T x)
{
	if constexpr (std::is_same_v<std::remove_cv_t<std::remove_reference_t<T>>, char*> || std::is_same_v<std::remove_cv_t<std::remove_reference_t<T>>, const char*> || std::is_same_v<std::remove_cv_t<std::remove_reference_t<T>>, std::string> || std::is_same_v<std::remove_cv_t<std::remove_reference_t<T>>, std::string_view>)
	{
		printText(x);
	}
	else
	{
		if constexpr (std::is_same_v<T, char>)
		{
			if (options::indent.hasIndentOnEmpty() || x != '\n')
				insertIndent();
			if (x == '\n')
				newLine = true;
			else if (sanitization && (x == '"' || x == '\\'))
				*o << '\\';
		}
		else
		{
			insertIndent();
		}
		*o << std::forward<T>(x);
	}
	return *this;
}
