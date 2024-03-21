#pragma once


// An object of this class casts to `true` the first time and to `false` every other time.
class First
{
	bool first = true;
public:
	[[nodiscard]] operator bool() { const bool firstCopy = first; first = false; return firstCopy; }
};
