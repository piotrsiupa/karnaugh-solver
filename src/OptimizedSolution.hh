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
	
	using id_t = std::size_t;
	using ids_t = std::vector<id_t>;
	using product_t = std::pair<PrimeImplicant, ids_t>;
	using sum_t = ids_t;
	
	Minterm negatedInputs = 0;
	std::vector<product_t> products;
	std::vector<sum_t> sums;
	std::vector<id_t> finalSums;
	
	void printNegatedInputs(std::ostream &o) const;
	void printProducts(std::ostream &o) const;
	void printSums(std::ostream &o) const;
	void printFinalSums(std::ostream &o, const strings_t &functionNames) const;
	void printGateScores(std::ostream &o) const;
	
	static void initializeWips(const solutions_t &solutions, wipProducts_t &wipProducts, wipSums_t &wipSums, wipFinalSums_t &wipFinalSums);
	static void extractCommonParts(wipProducts_t &wipProducts);
	static void extractCommonParts(wipSums_t &wipSums);
	void createNegatedInputs(const solutions_t &solutions);
	static id_t findWipProductId(const wipProducts_t &wipProducts, const ref_t wipProductRef);
	id_t findWipSumId(const wipSums_t &wipSums, const ref_t wipSumRef) const;
	void insertWipProducts(const wipProducts_t &wipProducts);
	void insertWipSums(const wipProducts_t &wipProducts, const wipSums_t &wipSums);
	void insertWipFinalSums(const wipSums_t &wipSums, const wipFinalSums_t &wipFinalSums);
	void cleanupProducts();
	void cleanupSums();
	
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
	std::size_t getNotCount() const { return __builtin_popcount(negatedInputs); }
	std::size_t getAndCount() const { std::size_t andCount = 0; for (const auto &[primeImplicant, ids] : products) andCount += std::max(std::size_t(1), primeImplicant.getBitCount() + ids.size()) - 1; return andCount; }
	std::size_t getOrCount() const { std::size_t orCount = 0; for (const auto &sum : sums) orCount += sum.size() - 1; return orCount; }
	std::size_t getGateScore() const { return getNotCount() + 2 * getAndCount() + 2 * getOrCount(); }
	
	void print(std::ostream &o, const strings_t &functionNames) const;
	
	static OptimizedSolution create(const solutions_t &solutions);
};
