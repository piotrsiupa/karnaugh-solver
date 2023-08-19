#pragma once

#include <cstddef>
#include <cstdint>
#include <ostream>
#include <vector>

#include "./Karnaugh.hh"


class Karnaugh_Solution
{
	using minterm_t = Karnaugh::minterm_t;
	using minterms_t = Karnaugh::minterms_t;
	using number_t = Karnaugh::number_t;
	using numbers_t = Karnaugh::numbers_t;
	
public:
	struct OptimizedSolution
	{
		number_t negatedInputs = 0;
		std::vector<std::pair<minterm_t, std::vector<std::size_t>>> products;
		std::vector<std::vector<std::size_t>> sums;
		std::vector<std::size_t> finalSums;
		
		void print(const bits_t bits, std::ostream &o, const names_t &inputNames, const names_t &functionNames) const;
		std::size_t getNotCount() const { return __builtin_popcount(negatedInputs); }
		std::size_t getAndCount() const { std::size_t andCount = 0; for (const auto &product : products) andCount += std::max(std::size_t(1), product.first.getBitCount() + product.second.size()) - 1; return andCount; }
		std::size_t getOrCount() const { std::size_t orCount = 0; for (const auto &sum : sums) orCount += sum.size() - 1; return orCount; }
		std::size_t getGateScore() const { return getNotCount() + 2 * getAndCount() + 2 * getOrCount(); }
	};
	
private:
	minterms_t minterms;
	numbers_t target;
	std::vector<minterms_t> solutions;
	
	Karnaugh_Solution(const minterms_t &minterms, const numbers_t &target);
	
	minterms_t removeEssentials();
	void solve();
	
public:
	static Karnaugh_Solution solve(const minterms_t &allMinters, const numbers_t &target);
	
	static void prettyPrintSolution(const bits_t bits, const minterms_t &solution);
	const std::vector<minterms_t>& getSolutions() const { return solutions; }
	
	static OptimizedSolution optimizeSolutions(const std::vector<const minterms_t*> &solutions);
};
