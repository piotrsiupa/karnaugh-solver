#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "./Karnaugh.hh"


template<bits_t BITS>
class Karnaugh_Solution
{
	using minterms_t = typename Karnaugh<BITS>::minterms_t;
	using table_t = typename Karnaugh<BITS>::table_t;
	using number_t = typename Karnaugh<BITS>::number_t;
	using mintermTables_t = std::vector<table_t>;
	using solution_t = std::vector<std::size_t>;
	using timePoint_t = std::chrono::time_point<std::chrono::steady_clock>;
	using elapsedTime_t = std::chrono::duration<double>;
	using progressCounter_t = std::uintmax_t;
	
	const Karnaugh<BITS> &karnaugh;
	mintermTables_t mintermTables;
	solution_t best;
	const double solutionSpaceSize;
	timePoint_t startTime, bestTime;
	
	Karnaugh_Solution(const Karnaugh<BITS> &karnaugh);
	
	static bool adjustProgressInterval(const elapsedTime_t elapsedTime, progressCounter_t &progressInterval);
	static double estimateRemainingSolutionsFactor(const solution_t &currentSolution);
	void displayProgress(const solution_t &currentSolution, const timePoint_t &currentTime, const elapsedTime_t elapsedTime) const;
	bool processProgress(const solution_t &currentSolution, progressCounter_t &progressInterval, bool &progressIntervalAdjusted) const;
	static void clearProgress();
	
	void createMintermTables();
	void removeUnnededMinterms(solution_t &current, mintermTables_t &currentTables);
	void solve();
	
public:
	static Karnaugh_Solution solve(const Karnaugh<BITS> &karnaugh);
	
	bool isBestFitValid() const { return best.size() < 2 || best[0] != best[1]; }
	void prettyPrintBestFit() const;
	minterms_t getBestMinterms() const;
};
