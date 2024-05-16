#pragma once

#include <bitset>
#include <cstddef>
#include <ostream>
#include <set>
#include <string_view>
#include <utility>
#include <vector>

#include "GateCost.hh"
#include "Implicant.hh"
#include "Names.hh"
#include "Progress.hh"
#include "Solution.hh"


class OptimizedSolutions final : public GateCost
{
public:
	using solutions_t = std::vector<const Solution*>;
	
private:
	using id_t = std::size_t;
	using ids_t = std::vector<id_t>;
	using product_t = struct { Implicant implicant; ids_t subProducts; };
	using sum_t = ids_t;
	using finalPrimeImplicants_t = std::vector<std::size_t>;
	
	Minterm negatedInputs = 0;
	std::vector<product_t> products;
	std::vector<sum_t> sums;
	std::vector<id_t> finalSums;
	
	mutable std::vector<id_t> normalizedIds;
	
	Implicant flattenProduct(const id_t productId) const;
	std::vector<id_t> flattenSum(const id_t sumId) const;
	std::size_t getIdUseCount(const id_t id) const { return std::accumulate(products.cbegin(), products.cend(), 0, [id](const std::size_t &acc, const product_t &product){ return acc + std::count(product.subProducts.cbegin(), product.subProducts.cend(), id); }) + std::accumulate(sums.cbegin(), sums.cend(), 0, [id](const std::size_t &acc, const sum_t &sum){ return acc + std::count(sum.cbegin(), sum.cend(), id); }); }
	bool isWorthPrinting(const id_t id, const bool simpleFinalSums) const { return isProduct(id) ? isProductWorthPrinting(id) : isSumWorthPrinting(id, simpleFinalSums); }
	bool isWorthPrintingOnGraph(const id_t id, const bool isFullGraph) const { return isProduct(id) ? isProductWorthPrintingOnGraph(id, isFullGraph) : isSumWorthPrintingOnGraph(id, isFullGraph); }
	void generateHumanIds() const;
	void generateGraphIds() const;
	std::pair<std::size_t, std::size_t> generateMathIds() const;
	void printHumanAnd(std::ostream &o) const;
	void printHumanOr(std::ostream &o) const;
	void printGraphNot(std::ostream &o) const;
	void printGraphAnd(std::ostream &o) const;
	void printGraphOr(std::ostream &o, const bool spaces) const;
	std::pair<std::size_t, std::size_t> generateNormalizedIds() const;
	void printVerilogImmediates(std::ostream &o, const std::size_t immediateProductCount, const std::size_t immediateSumCount) const;
	void printVhdlImmediates(std::ostream &o, const std::size_t immediateProductCount, const std::size_t immediateSumCount) const;
	void printCppImmediates(std::ostream &o, const std::size_t immediateProductCount, const std::size_t immediateSumCount) const;
	void printHumanId(std::ostream &o, const id_t id) const;
	void printGraphId(std::ostream &o, const id_t id) const;
	void printVerilogId(std::ostream &o, const id_t id) const;
	void printVhdlId(std::ostream &o, const id_t id) const;
	void printCppId(std::ostream &o, const id_t id) const;
	void printMathArgs(std::ostream &o, const id_t id) const;
	void printMathId(std::ostream &o, const id_t id) const;
	void printHumanNegatedInputs(std::ostream &o) const;
	void printGraphNegatedInputs(std::ostream &o) const;
	bool isProductWorthPrinting(const id_t productId) const { const product_t &product = getProduct(productId); return product.implicant.getBitCount() >= 2 || !product.subProducts.empty(); }
	void printHumanProductBody(std::ostream &o, const id_t productId) const;
	void printHumanProduct(std::ostream &o, const id_t productId) const;
	void printHumanProducts(std::ostream &o) const;
	std::size_t findProductEndNode(const id_t productId, std::size_t startFunctionNum = 0) const;
	bool isProductWorthPrintingOnGraph(const id_t productId, const bool isFullGraph) const { return isProductWorthPrinting(productId) || (!isFullGraph && findProductEndNode(productId) != SIZE_MAX); }
	void printGraphProductImplicant(std::ostream &o, const id_t productId) const;
	void printGraphProductParents(std::ostream &o, const id_t productId) const;
	void printGraphProduct(std::ostream &o, const Names &functionNames, const id_t productId) const;
	void printGraphProducts(std::ostream &o, const Names &functionNames) const;
	void printVerilogProductBody(std::ostream &o, const id_t productId) const;
	void printVerilogProduct(std::ostream &o, const id_t productId) const;
	void printVerilogProducts(std::ostream &o) const;
	void printVhdlProductBody(std::ostream &o, const id_t productId) const;
	void printVhdlProduct(std::ostream &o, const id_t productId) const;
	void printVhdlProducts(std::ostream &o) const;
	void printCppProductBody(std::ostream &o, const id_t productId) const;
	void printCppProduct(std::ostream &o, const id_t productId) const;
	void printCppProducts(std::ostream &o) const;
	bool isProductWorthPrintingForMath(const id_t productId) const { return isProductWorthPrinting(productId) && getIdUseCount(productId) >= 2; }
	void printMathProductBody(std::ostream &o, const id_t productId, const bool parentheses) const;
	void printMathProduct(std::ostream &o, const id_t productId) const;
	void printMathProducts(std::ostream &o) const;
	bool isSumWorthPrinting(const id_t sumId, const bool simpleFinalSums) const;
	void printHumanSumBody(std::ostream &o, const id_t sumId) const;
	void printHumanSum(std::ostream &o, const id_t sumId) const;
	void printHumanSums(std::ostream &o) const;
	std::size_t findSumEndNode(const id_t sumId, const std::size_t startFunctionNum = 0) const;
	bool isSumWorthPrintingOnGraph(const id_t sumId, const bool isFullGraph) const { return isFullGraph ? isSumWorthPrinting(sumId, false) : getSum(sumId).size() != 1; }
	void printGraphSumProducts(std::ostream &o, const id_t sumId) const;
	void printGraphSumParents(std::ostream &o, const id_t sumId) const;
	void printGraphSum(std::ostream &o, const Names &functionNames, const id_t sumId) const;
	void printGraphSums(std::ostream &o, const Names &functionNames) const;
	void printVerilogSumBody(std::ostream &o, const id_t sumId) const;
	void printVerilogSum(std::ostream &o, const id_t sumId) const;
	void printVerilogSums(std::ostream &o) const;
	void printVhdlSumBody(std::ostream &o, const id_t sumId) const;
	void printVhdlSum(std::ostream &o, const id_t sumId) const;
	void printVhdlSums(std::ostream &o) const;
	void printCppSumBody(std::ostream &o, const id_t sumId) const;
	void printCppSum(std::ostream &o, const id_t sumId) const;
	void printCppSums(std::ostream &o) const;
	bool isSumWorthPrintingForMath(const id_t sumId) const { return isSumWorthPrinting(sumId, false) && (getIdUseCount(sumId) + std::count(finalSums.cbegin(), finalSums.cend(), sumId) >= 2); }
	void printMathSumBody(std::ostream &o, const id_t sumId) const;
	void printMathSum(std::ostream &o, const id_t sumId) const;
	void printMathSums(std::ostream &o) const;
	void printHumanFinalSums(std::ostream &o, const Names &functionNames) const;
	void printGraphFinalSum(std::ostream &o, const Names &functionNames, const std::size_t i) const;
	void printGraphFinalSums(std::ostream &o, const Names &functionNames) const;
	void printVerilogFinalSums(std::ostream &o, const Names &functionNames) const;
	void printVhdlFinalSums(std::ostream &o, const Names &functionNames) const;
	void printCppFinalSums(std::ostream &o, const Names &functionNames) const;
	void printMathFinalSums(std::ostream &o, const Names &functionNames) const;
	
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
	void validate(const solutions_t &solutions, Progress &progress) const;
#endif
	
public:
	OptimizedSolutions() = default;
	OptimizedSolutions(const solutions_t &solutions, Progress &progress);
	
	std::size_t getSize() const { return finalSums.size(); }
	
	std::size_t getNotCount() const final { return std::bitset<32>(negatedInputs).count(); }
	std::size_t getAndCount() const final { std::size_t andCount = 0; for (const auto &[primeImplicant, ids] : products) andCount += std::max(std::size_t(1), primeImplicant.getBitCount() + ids.size()) - 1; return andCount; }
	std::size_t getOrCount() const final { std::size_t orCount = 0; for (const auto &sum : sums) orCount += sum.size() - 1; return orCount; }
	
	std::pair<bool, bool> checkForUsedConstants() const;
	void printHuman(std::ostream &o, const Names &functionNames) const;
	void printGraph(std::ostream &o, const Names &functionNames) const;
	void printVerilog(std::ostream &o, const Names &functionNames) const;
	void printVhdl(std::ostream &o, const Names &functionNames) const;
	void printCpp(std::ostream &o, const Names &functionNames) const;
	void printMath(std::ostream &o, const Names &functionNames) const;
};
