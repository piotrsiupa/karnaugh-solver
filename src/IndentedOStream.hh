#pragma once

#include <cassert>
#include <cstddef>
#include <ostream>
#include <utility>
#include <type_traits>


class IndentedOStream
{
	std::ostream *o;
	std::size_t indentSize = 0;
	bool newLine = true;
	
	void insertIndent();
	void printText(const std::string_view text);
	
public:
	IndentedOStream() : o(nullptr) {}
	IndentedOStream(std::ostream &o) : o(&o) {}
	
	template<typename T>
	IndentedOStream& operator<<(T x);
	[[nodiscard]] operator std::ostream&() { insertIndent(); return *o; }
	IndentedOStream operator=(std::ostream &newO) { o = &newO; return *this; }
	
	void indent(const std::size_t x = 1) { assert(SIZE_MAX - indentSize >= x); indentSize += x; }
	void deindent(const std::size_t x = 1) { assert(indentSize >= x); indentSize -= x; }
	class IndentGuard
	{
		IndentedOStream &stream;
		std::size_t size;
		IndentGuard(IndentedOStream &stream, const std::size_t x) : stream(stream), size(x) { stream.indent(x); }
		friend class IndentedOStream;
	public:
		IndentGuard(const IndentGuard &) = delete;
		IndentGuard& operator=(const IndentGuard &) = delete;
		~IndentGuard() { reset(); }
		void increase(const std::size_t x = 1) { size += x; stream.indent(x); }
		void reset() { stream.deindent(size); size = 0; }
	};
	[[nodiscard]] IndentGuard startIndent(const std::size_t x = 1) { return {*this, x}; }
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
		insertIndent();
		*o << std::forward<T>(x);
		if constexpr (std::is_same_v<T, char>)
			if (x == '\n')
				newLine = true;
	}
	return *this;
}
