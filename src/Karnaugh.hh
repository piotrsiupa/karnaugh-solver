#pragma once

#include <memory>
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
	using duplicates_t = std::vector<Minterm>;
	
	static std::size_t nameCount;
	
	bool nameIsCustom = false;
	std::string functionName;
	std::shared_ptr<Minterms> targetMinterms, allowedMinterms;
	
	static void printDuplicates(const duplicates_t &duplicates, Progress::CerrGuard &cerr);
	
	static bool isTableSmallEnoughToPrint() { return ::bits <= 8; }
	static grayCode_t makeGrayCode(const bits_t bitCount);
	static void printBits(const Minterm minterm, const bits_t bitCount);
	static void prettyPrintTable(const Minterms &target, const Minterms &allowed = {});
	void prettyPrintTable() const;
	static void prettyPrintSolution(const Solution &solution);
	
	class MintermLoadingCompletionCalculator {
		const Minterms &minterms;
		const Minterm &currentMinterm;
		const bool dontCares;
		const std::size_t estimatedSize;
		Minterm lastMinterm = 0;
		bool inOrderSoFar = true;
	public:
		MintermLoadingCompletionCalculator(const Minterms &minterms, const Minterm &currentMinterm, const bool dontCares, const std::size_t estimatedSize = 0) : minterms(minterms), currentMinterm(currentMinterm), dontCares(dontCares), estimatedSize(estimatedSize) {}
		Progress::completion_t operator()();
	};
	static std::size_t estimateRemainingInputSize(Input &input);
	std::unique_ptr<Minterms> loadMinterms(Input &input, Progress &progress, const bool dontCares) const;
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
	Solutions solve() &&;  // This function is `&&` as a reminder the it removes some data in the process (to save memory) and because of that it cannot be called twice.
	
	void printHumanSolution(const Solution &solution) const;
	void printVerilogSolution(const Solution &solution) const;
	void printVhdlSolution(const Solution &solution) const;
	void printCppSolution(const Solution &solution) const;
	void printMathSolution(const Solution &solution) const;
};
