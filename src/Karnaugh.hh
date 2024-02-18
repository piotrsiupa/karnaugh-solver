#pragma once

#include <string>
#include <vector>

#include "global.hh"
#include "Input.hh"
#include "Minterm.hh"
#include "Minterms.hh"
#include "Progress.hh"
#include "Solution.hh"
#include "Solutions.hh"


class Karnaugh
{
	using grayCode_t = std::vector<Minterm>;
	
	static std::size_t nameCount;
	
	bool nameIsCustom = false;
	std::string functionName;
	Minterms targetMinterms, allowedMinterms;
	
	static grayCode_t makeGrayCode(const bits_t bitCount);
	static void printBits(const Minterm minterm, const bits_t bitCount);
	static void prettyPrintTable(const Minterms &target, const Minterms &allowed = {});
	void prettyPrintTable() const;
	static void prettyPrintSolution(const Solution &solution);
	
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
	
	bool loadData(Input &input);
	Solutions solve() const;
	
	void printHumanSolution(const Solution &solution) const;
	void printVerilogSolution(const Solution &solution) const;
	void printVhdlSolution(const Solution &solution) const;
	void printCppSolution(const Solution &solution) const;
	void printMathSolution(const Solution &solution) const;
};
