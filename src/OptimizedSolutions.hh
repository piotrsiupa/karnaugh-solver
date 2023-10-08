#pragma once

#include <cstddef>
#include <ostream>
#include <set>
#include <utility>
#include <vector>

#include "PrimeImplicant.hh"
#include "PrimeImplicants.hh"


class OptimizedSolutions
{
public:
	using solutions_t = std::vector<const PrimeImplicants*>;
	
private:
	using id_t = std::size_t;
	using ids_t = std::vector<id_t>;
	using product_t = std::pair<PrimeImplicant, ids_t>;
	using sum_t = ids_t;
	using finalPrimeImplicants_t = std::vector<std::size_t>;
	
	Minterm negatedInputs = 0;
	std::vector<product_t> products;
	std::vector<sum_t> sums;
	std::vector<id_t> finalSums;
	
	void printNegatedInputs(std::ostream &o) const;
	void printProducts(std::ostream &o) const;
	void printSums(std::ostream &o) const;
	void printFinalSums(std::ostream &o, const strings_t &functionNames) const;
	void printGateScores(std::ostream &o) const;
	
	void createNegatedInputs(const solutions_t &solutions);
	finalPrimeImplicants_t extractCommonParts(const solutions_t &solutions);
	void extractCommonParts(const solutions_t &solutions, const finalPrimeImplicants_t &finalPrimeImplicants);
	
	static id_t makeProductId(const std::size_t index) { return index; }
	id_t makeSumId(const std::size_t index) const { return index + products.size(); }
	bool isProduct(const id_t id) const { return id < products.size(); }
	const product_t& getProduct(const id_t id) const { return products[id]; }
	const sum_t& getSum(const id_t id) const { return sums[id - products.size()]; }
	
#ifndef NDEBUG
	using normalizedSolution_t = std::set<PrimeImplicant>;
	normalizedSolution_t normalizeSolution(const id_t finalSumId) const;
	void validate(const solutions_t &solutions) const;
#endif
	
public:
	OptimizedSolutions() = default;
	OptimizedSolutions(const solutions_t &solutions);
	
	std::size_t getSize() const { return finalSums.size(); }
	
	std::size_t getNotCount() const { return __builtin_popcount(negatedInputs); }
	std::size_t getAndCount() const { std::size_t andCount = 0; for (const auto &[primeImplicant, ids] : products) andCount += std::max(std::size_t(1), primeImplicant.getBitCount() + ids.size()) - 1; return andCount; }
	std::size_t getOrCount() const { std::size_t orCount = 0; for (const auto &sum : sums) orCount += sum.size() - 1; return orCount; }
	std::size_t getGateScore() const { return getNotCount() + 2 * getAndCount() + 2 * getOrCount(); }
	
	void print(std::ostream &o, const strings_t &functionNames) const;
};
