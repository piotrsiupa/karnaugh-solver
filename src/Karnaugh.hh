#pragma once

#include <string>
#include <vector>

#include "Input.hh"
#include "Minterms.hh"
#include "Progress.hh"
#include "Solutions.hh"


class Karnaugh
{
	static std::size_t nameCount;
	
	bool nameIsCustom = false;
	std::string functionName;
	Minterms targetMinterms, allowedMinterms;
	
	bool loadMinterms(Minterms &minterms, Input &input, Progress &progress) const;
#ifndef NDEBUG
	void validate(const Solutions &solutions) const;
#endif
	
public:
	Karnaugh() : functionName('f' + std::to_string(nameCount++)) {}
	Karnaugh(Karnaugh &&) = default;
	Karnaugh& operator=(Karnaugh &&) = default;
	
	bool hasCustomName() const { return nameIsCustom; }
	const std::string& getFunctionName() const { return functionName; }
	const Minterms& getTargetMinterms() const { return targetMinterms; }
	const Minterms& getAllowedMinterms() const { return allowedMinterms; }
	
	bool loadData(Input &input);
	Solutions solve() const;
};
