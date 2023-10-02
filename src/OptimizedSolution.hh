#pragma once

#include <cstddef>
#include <ostream>
#include <map>
#include <set>
#include <utility>
#include <vector>

#include "PrimeImplicant.hh"
#include "PrimeImplicants.hh"


class OptimizedSolution
{
public:
	using solutions_t = std::vector<const PrimeImplicants*>;
	
private:
	using ref_t = const void*;
	static bool wipSumsLess(const std::set<ref_t> &x, const std::set<ref_t> &y) { return x.size() != y.size() ? x.size() < y.size() : x < y; }
	using wipProducts_t = std::map<PrimeImplicant, std::vector<ref_t>>;
	using wipSums_t = std::map<std::set<ref_t>, std::vector<ref_t>, decltype(wipSumsLess)&>;
	using wipFinalSums_t = std::vector<ref_t>;
	
	Minterm negatedInputs = 0;
	std::vector<std::pair<PrimeImplicant, std::vector<std::size_t>>> products;
	std::vector<std::vector<std::size_t>> sums;
	std::vector<std::size_t> finalSums;
	
	void printNegatedInputs(std::ostream &o) const;
	void printProducts(std::ostream &o) const;
	void printSums(std::ostream &o) const;
	void printFinalSums(std::ostream &o, const strings_t &functionNames) const;
	void printGateScores(std::ostream &o) const;
	
	static void initializeWips(const solutions_t &solutions, wipProducts_t &wipProducts, wipSums_t &wipSums, wipFinalSums_t &wipFinalSums);
	static void extractCommonParts(wipProducts_t &wipProducts);
	static void extractCommonParts(wipSums_t &wipSums);
	void createNegatedInputs(const solutions_t &solutions);
	static std::size_t findWipProductRefIndex(const wipProducts_t &wipProducts, const ref_t wipProductRef);
	std::size_t findWipSumRefIndex(const wipSums_t &wipSums, const ref_t wipSumRef) const;
	void insertWipProducts(const wipProducts_t &wipProducts);
	void insertWipSums(const wipProducts_t &wipProducts, const wipSums_t &wipSums);
	void insertWipFinalSums(const wipSums_t &wipSums, const wipFinalSums_t &wipFinalSums);
	void cleanupProducts();
	void cleanupSums();
	
public:
	std::size_t getNotCount() const { return __builtin_popcount(negatedInputs); }
	std::size_t getAndCount() const { std::size_t andCount = 0; for (const auto &[primeImplicant, references] : products) andCount += std::max(std::size_t(1), primeImplicant.getBitCount() + references.size()) - 1; return andCount; }
	std::size_t getOrCount() const { std::size_t orCount = 0; for (const auto &sum : sums) orCount += sum.size() - 1; return orCount; }
	std::size_t getGateScore() const { return getNotCount() + 2 * getAndCount() + 2 * getOrCount(); }
	
	void print(std::ostream &o, const strings_t &functionNames) const;
	
	static OptimizedSolution create(const solutions_t &solutions);
};
