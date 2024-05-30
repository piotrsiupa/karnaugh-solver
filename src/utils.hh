#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <numeric>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>


// An object of this class casts to `true` the first time and to `false` every other time.
class First
{
	bool first = true;
public:
	[[nodiscard]] operator bool() { const bool firstCopy = first; first = false; return firstCopy; }
};


class Trilean
{
public:
	enum class Value : std::uint_fast8_t
	{
		FALSE = false,
		TRUE = true,
		UNDEFINED = 2,
	};
	
private:
	Value value;
	
public:
	Trilean(const Trilean &other) = default;
	Trilean(const bool value) : value(static_cast<Value>(value)) {}
	Trilean(const Value value) : value(value) {}
	
	[[nodiscard]] bool operator==(const Trilean &other) const { return this->value == other.value; }
	[[nodiscard]] bool operator==(const bool newValue) const { return this->value == static_cast<Value>(newValue); }
	[[nodiscard]] bool operator==(const Value newValue) const { return this->value == newValue; }
	
	[[nodiscard]] Value get() const { return value; }
	
	[[nodiscard]] static Trilean fromUndefinedAndBool(const bool isUndefined, const bool value) { return isUndefined ? Value::UNDEFINED : static_cast<Value>(value); }
	[[nodiscard]] static Trilean fromTrueAndFalse(const bool isTrue, const bool isFalse) { return isTrue ? Value::TRUE : isFalse ? Value::FALSE : Value::UNDEFINED; }
};


[[nodiscard]] inline std::string joinStrings(const std::vector<std::string> strings, const std::string_view join = ", ", const std::string_view prefix = "", const std::string_view suffix = "")
{
	std::string result;
	if (!strings.empty())
	{
		const std::size_t calculatedSize = std::accumulate(strings.cbegin(), strings.cend(), std::size_t(0), [](const std::size_t acc, const std::string &x){ return acc + x.size(); }) + strings.size() * (prefix.size() + suffix.size()) + (strings.size() - 1) * join.size();
		result.reserve(calculatedSize);
		First first;
		for (const std::string &string : strings)
		{
			if (!first)
				result += join;
			result += prefix;
			result += string;
			result += suffix;
		}
		assert(result.size() == calculatedSize);
	}
	return result;
}


template<typename OT, typename IT, class OP>
[[nodiscard]] std::vector<OT> map_vector(const std::vector<IT> &in, OP op)
{
	std::vector<OT> out;
	out.reserve(in.size());
	std::transform(in.cbegin(), in.cend(), std::back_inserter(out), op);
	return out;
}


template<typename T>
inline constexpr bool is_nullable = std::is_null_pointer_v<T> || std::is_pointer_v<T>;
