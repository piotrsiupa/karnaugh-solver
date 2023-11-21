#pragma once

#include <bitset>
#include <cstddef>
#include <ostream>
#include <set>
#include <string_view>
#include <utility>
#include <vector>

#include "Implicant.hh"
#include "Implicants.hh"
#include "Names.hh"
#include "Progress.hh"


class OptimizedSolutions
{
public:
	using solutions_t = std::vector<const Implicants*>;
	
private:
	using id_t = std::size_t;
	using ids_t = std::vector<id_t>;
	using product_t = std::pair<Implicant, ids_t>;
	using sum_t = ids_t;
	using finalPrimeImplicants_t = std::vector<std::size_t>;
	
	Minterm negatedInputs = 0;
	std::vector<product_t> products;
	std::vector<sum_t> sums;
	std::vector<id_t> finalSums;
	
	mutable std::vector<id_t> normalizedIds;
	
	bool isWorthPrinting(const id_t id, const bool simpleFinalSums) const { return isProduct(id) ? isProductWorthPrinting(id) : isSumWorthPrinting(id, simpleFinalSums); }
	std::size_t generateHumanIds() const;
	std::pair<std::size_t, std::size_t> generateNormalizedIds() const;
	void printVerilogImmediates(std::ostream &o, const std::size_t immediateProductCount, const std::size_t immediateSumCount) const;
	void printHumanId(std::ostream &o, const id_t id) const;
	void printVerilogId(std::ostream &o, const id_t id) const;
	void printHumanNegatedInputs(std::ostream &o) const;
	bool isProductWorthPrinting(const id_t productId) const { const product_t &product = getProduct(productId); return product.first.getBitCount() >= 2 || !product.second.empty(); }
	void printHumanProductBody(std::ostream &o, const id_t productId) const;
	void printHumanProduct(std::ostream &o, const id_t productId) const;
	void printHumanProducts(std::ostream &o) const;
	void printVerilogProductBody(std::ostream &o, const id_t productId) const;
	void printVerilogProduct(std::ostream &o, const id_t productId) const;
	void printVerilogProducts(std::ostream &o) const;
	bool isSumWorthPrinting(const id_t sumId, const bool simpleFinalSums) const { if (simpleFinalSums) return getSum(sumId).size() >= 2; for (const sum_t &sum : sums) for (const id_t &id : sum) if (id == sumId) return true; return false; }
	void printHumanSumBody(std::ostream &o, const id_t sumId) const;
	void printHumanSum(std::ostream &o, const id_t sumId) const;
	void printHumanSums(std::ostream &o) const;
	void printVerilogSumBody(std::ostream &o, const id_t sumId) const;
	void printVerilogSum(std::ostream &o, const id_t sumId) const;
	void printVerilogSums(std::ostream &o) const;
	void printHumanFinalSums(std::ostream &o, const Names &functionNames) const;
	void printVerilogFinalSums(std::ostream &o, const Names &functionNames) const;
	void printGateScores(std::ostream &o) const;
	
	void createNegatedInputs(const solutions_t &solutions);
	finalPrimeImplicants_t extractCommonProductParts(const solutions_t &solutions, Progress &progress);
	void extractCommonSumParts(const solutions_t &solutions, const finalPrimeImplicants_t &finalPrimeImplicants, Progress &progress);
	
	static id_t makeProductId(const std::size_t index) { return index; }
	id_t makeSumId(const std::size_t index) const { return index + products.size(); }
	bool isProduct(const id_t id) const { return id < products.size(); }
	const product_t& getProduct(const id_t id) const { return products[id]; }
	const sum_t& getSum(const id_t id) const { return sums[id - products.size()]; }
	
#ifndef NDEBUG
	using normalizedSolution_t = std::set<Implicant>;
	normalizedSolution_t normalizeSolution(const id_t finalSumId) const;
	void validate(const solutions_t &solutions) const;
#endif
	
public:
	OptimizedSolutions() = default;
	OptimizedSolutions(const solutions_t &solutions, Progress &progress);
	
	std::size_t getSize() const { return finalSums.size(); }
	
	std::size_t getNotCount() const { return std::bitset<32>(negatedInputs).count(); }
	std::size_t getAndCount() const { std::size_t andCount = 0; for (const auto &[primeImplicant, ids] : products) andCount += std::max(std::size_t(1), primeImplicant.getBitCount() + ids.size()) - 1; return andCount; }
	std::size_t getOrCount() const { std::size_t orCount = 0; for (const auto &sum : sums) orCount += sum.size() - 1; return orCount; }
	std::size_t getGateScore() const { return getNotCount() + 2 * getAndCount() + 2 * getOrCount(); }
	
	void printHuman(std::ostream &o, const Names &functionNames) const;
	void printVerilog(std::ostream &o, const Names &functionNames) const;
};
