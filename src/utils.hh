#pragma once

#include <algorithm>
#include <vector>


// An object of this class casts to `true` the first time and to `false` every other time.
class First
{
	bool first = true;
public:
	[[nodiscard]] operator bool() { const bool firstCopy = first; first = false; return firstCopy; }
};


template<typename OT, typename IT, class OP>
std::vector<OT> map_vector(const std::vector<IT> &in, OP op)
{
	std::vector<OT> out;
	out.reserve(in.size());
	std::transform(in.cbegin(), in.cend(), std::back_inserter(out), op);
	return out;
}
