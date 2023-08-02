#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "./Karnaugh.hh"


template<bits_t BITS>
class Karnaugh_Solution
{
	using minterm_t = typename Karnaugh<BITS>::minterm_t;
	using minterms_t = typename Karnaugh<BITS>::minterms_t;
	using table_t = typename Karnaugh<BITS>::table_t;
	using number_t = typename Karnaugh<BITS>::number_t;
	using mintermTables_t = std::vector<table_t>;
	using solution_t = std::vector<std::size_t>;
	using timePoint_t = std::chrono::time_point<std::chrono::steady_clock>;
	using elapsedTime_t = std::chrono::duration<double>;
	using progressCounter_t = std::uintmax_t;
	
	minterms_t minterms;
	table_t target;
	const table_t &dontCares;
	mintermTables_t mintermTables;
	solution_t best;
	const double solutionSpaceSize;
	timePoint_t startTime, bestTime;
	minterms_t solution;
	
	Karnaugh_Solution(const minterms_t &minterms, const table_t &target, const table_t &dontCares);
	
	static bool adjustProgressInterval(const elapsedTime_t elapsedTime, progressCounter_t &progressInterval);
	static double estimateRemainingSolutionsFactor(const solution_t &currentSolution);
	void displayProgress(const solution_t &currentSolution, const timePoint_t &currentTime, const elapsedTime_t elapsedTime) const;
	bool processProgress(const solution_t &currentSolution, progressCounter_t &progressInterval, bool &progressIntervalAdjusted) const;
	static void clearProgress();
	
	static table_t createMintermTable(const minterm_t minterm);
	void createMintermTables();
	minterms_t removeEssentials();
	void removeUnnededMinterms(solution_t &current, mintermTables_t &currentTables);
	void solve();
	
public:
	static Karnaugh_Solution solve(const minterms_t &allMinters, const table_t &target, const table_t &dontCares);
	
	bool isSolutionValid() const { return solution.size() < 2 || solution[0] != solution[1]; }
	void prettyPrintSolution() const;
	const minterms_t& getSolution() const { return solution; }
};
