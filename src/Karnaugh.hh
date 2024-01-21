#pragma once

#include <memory>
#include <string>
#include <vector>

#include "global.hh"
#include "Implicant.hh"
#include "Implicants.hh"
#include "Input.hh"
#include "Minterm.hh"
#include "Minterms.hh"
#include "Progress.hh"


class Karnaugh
{
public:
	using solutions_t = std::vector<Implicants>;
	
private:
	using grayCode_t = std::vector<Minterm>;
	using duplicates_t = Minterms::duplicates_t;
	
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
	static void prettyPrintSolution(const Implicants &solution);
	
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
	void validate(const solutions_t &solutions) const;
#endif
	
public:
	Karnaugh() : functionName('f' + std::to_string(nameCount++)) {}
	Karnaugh(Karnaugh &&) = default;
	Karnaugh& operator=(Karnaugh &&) = default;
	
	bool hasCustomName() const { return nameIsCustom; }
	const std::string& getFunctionName() const { return functionName; }
	
	bool loadData(Input &input);
	solutions_t solve() const;
	
	void printHumanSolution(const Implicants &solution) const;
	void printVerilogSolution(const Implicants &solution) const;
	void printVhdlSolution(const Implicants &solution) const;
	void printCppSolution(const Implicants &solution) const;
	void printMathSolution(const Implicants &solution) const;
};
