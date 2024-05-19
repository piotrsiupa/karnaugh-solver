#pragma once

#include <ostream>
#include <string>
#include <string_view>
#include <vector>

#include "GateCost.hh"
#include "Karnaugh.hh"
#include "Names.hh"
#include "OptimizedSolutions.hh"
#include "Solution.hh"
#include "Solutions.hh"


class OutputComposer
{
	using grayCode_t = std::vector<Minterm>;
	
	const Names &functionNames;
	const std::vector<Karnaugh> &karnaughs;
	const Solutions &solutions;
	const OptimizedSolutions *const optimizedSolutions;
	
	[[nodiscard]] std::pair<bool, bool> checkForUsedConstants(const Solution &solution) const;
	[[nodiscard]] std::pair<bool, bool> checkForUsedConstants() const;
	[[nodiscard]] bool areInputsUsed() const;
	
	[[nodiscard]] bool isOptimizedProductWorthPrinting(const OptimizedSolutions::id_t productId) const { const OptimizedSolutions::product_t &product = optimizedSolutions->getProduct(productId); return product.implicant.getBitCount() >= 2 || !product.subProducts.empty(); }
	[[nodiscard]] bool isOptimizedProductWorthPrintingOnGraph(const OptimizedSolutions::id_t productId, const bool isFullGraph) const { return isOptimizedProductWorthPrinting(productId) || (!isFullGraph && optimizedSolutions->findProductEndNode(productId) != SIZE_MAX); }
	[[nodiscard]] bool isOptimizedProductWorthPrintingForMath(const OptimizedSolutions::id_t productId) const { return isOptimizedProductWorthPrinting(productId) && optimizedSolutions->getIdUseCount(productId) >= 2; }
	[[nodiscard]] bool isOptimizedSumWorthPrinting(const OptimizedSolutions::id_t sumId, const bool simpleFinalSums) const;
	[[nodiscard]] bool isOptimizedSumWorthPrintingOnGraph(const OptimizedSolutions::id_t sumId, const bool isFullGraph) const { return isFullGraph ? isOptimizedSumWorthPrinting(sumId, false) : optimizedSolutions->getSum(sumId).size() != 1; }
	[[nodiscard]] bool isOptimizedSumWorthPrintingForMath(const OptimizedSolutions::id_t sumId) const { return isOptimizedSumWorthPrinting(sumId, false) && (optimizedSolutions->getIdUseCount(sumId) + std::count(optimizedSolutions->getFinalSums().cbegin(), optimizedSolutions->getFinalSums().cend(), sumId) >= 2); }
	[[nodiscard]] bool isOptimizedPartWorthPrinting(const OptimizedSolutions::id_t id, const bool simpleFinalSums) const { return optimizedSolutions->isProduct(id) ? isOptimizedProductWorthPrinting(id) : isOptimizedSumWorthPrinting(id, simpleFinalSums); }
	[[nodiscard]] bool isOptimizedPartWorthPrintingOnGraph(const OptimizedSolutions::id_t id, const bool isFullGraph) const { return optimizedSolutions->isProduct(id) ? isOptimizedProductWorthPrintingOnGraph(id, isFullGraph) : isOptimizedSumWorthPrintingOnGraph(id, isFullGraph); }
	
	[[nodiscard]] static std::string getName();
	
	std::vector<OptimizedSolutions::id_t> normalizedOptimizedIds;
	
	void generateOptimizedHumanIds();
	void generateOptimizedGraphIds();
	[[nodiscard]] std::pair<std::size_t, std::size_t> generateOptimizedMathIds();
	[[nodiscard]] std::pair<std::size_t, std::size_t> generateOptimizedNormalizedIds();
	
	void printOptimizedHumanId(std::ostream &o, const OptimizedSolutions::id_t id) const;
	void printOptimizedGraphId(std::ostream &o, const OptimizedSolutions::id_t id) const;
	void printOptimizedVerilogId(std::ostream &o, const OptimizedSolutions::id_t id) const;
	void printOptimizedVhdlId(std::ostream &o, const OptimizedSolutions::id_t id) const;
	void printOptimizedCppId(std::ostream &o, const OptimizedSolutions::id_t id) const;
	void printOptimizedMathId(std::ostream &o, const OptimizedSolutions::id_t id) const;
	
	static void printBanner(std::ostream &o);
	
	static grayCode_t makeGrayCode(const bits_t bitCount);
	static void printBits(std::ostream &o, const Minterm minterm, const bits_t bitCount);
	static void prettyPrintTable(std::ostream &o, const Minterms &target, const Minterms &allowed = {});
	void prettyPrintTable(std::ostream &o, const std::size_t i) const;
	static void prettyPrintSolution(std::ostream &o, const Solution &solution);
	
	void printHumanBool(std::ostream &o, const bool value) const;
	void printHumanNot(std::ostream &o) const;
	void printGraphNot(std::ostream &o) const;
	void printHumanAnd(std::ostream &o) const;
	void printGraphAnd(std::ostream &o, const bool spaces) const;
	void printHumanOr(std::ostream &o, const bool spaces) const;
	void printGraphOr(std::ostream &o, const bool spaces) const;
	
	void printHumanImplicant(std::ostream &o, const Implicant &implicant, const bool parentheses) const;
	void printGraphImplicant(std::ostream &o, const Implicant &implicant, const bool parentheses) const;
	void printGraphImplicant(std::ostream &o, const Implicant &implicant) const;
	void printVerilogImplicant(std::ostream &o, const Implicant &implicant, const bool parentheses) const;
	void printVhdlImplicant(std::ostream &o, const Implicant &implicant, const bool parentheses) const;
	void printCppImplicant(std::ostream &o, const Implicant &implicant, const bool parentheses) const;
	void printMathImplicant(std::ostream &o, const Implicant &implicant, const bool parentheses) const;
	
	void printGateCost(std::ostream &o, const GateCost &gateCost, const bool full) const;
	
	void printGraphConstants(std::ostream &o) const;
	void printGraphRoots(std::ostream &o) const;
	void printGraphNegatedInputs(std::ostream &o, const Solution &solution, const std::size_t functionNum) const;
	void printGraphInputs(std::ostream &o) const;
	static void printGraphParentBit(std::ostream &o, const std::size_t functionNum, const Implicant::splitBit_t &splitBit, const std::size_t i);
	[[nodiscard]] std::size_t printGraphProducts(std::ostream &o, const Solution &solution, const std::size_t functionNum, std::size_t idShift) const;
	void printGraphSum(std::ostream &o, const Solution &solution, const std::size_t functionNum) const;
	
	void printHumanSolution_(std::ostream &o, const Solution &solution) const;
	[[nodiscard]] std::size_t printGraphSolution_(std::ostream &o, const Solution &solution, const std::size_t functionNum, const std::size_t idShift) const;
	void printVerilogSolution_(std::ostream &o, const Solution &solution) const;
	void printVhdlSolution_(std::ostream &o, const Solution &solution) const;
	void printCppSolution_(std::ostream &o, const Solution &solution) const;
	void printMathSolution_(std::ostream &o, const Solution &solution) const;
	
	void printHumanSolution(std::ostream &o, const std::size_t i) const;
	[[nodiscard]] std::size_t printGraphSolution(std::ostream &o, const std::size_t i, const std::size_t idShift) const;
	void printVerilogSolution(std::ostream &o, const Solution &solution) const;
	void printVhdlSolution(std::ostream &o, const Solution &solution) const;
	void printCppSolution(std::ostream &o, const Solution &solution) const;
	void printMathSolution(std::ostream &o, const Solution &solution) const;
	
	void printHumanSolutions(std::ostream &o) const;
	void printGraphSolutions(std::ostream &o) const;
	void printVerilogSolutions(std::ostream &o) const;
	void printVhdlSolutions(std::ostream &o, const Names &functionNames) const;
	void printCppSolutions(std::ostream &o, const Names &functionNames) const;
	void printMathSolutions(std::ostream &o, const Names &functionNames) const;
	
	void printOptimizedGraphProductImplicant(std::ostream &o, const OptimizedSolutions::id_t productId) const;
	void printOptimizedGraphProductParents(std::ostream &o, const OptimizedSolutions::id_t productId) const;
	
	void printOptimizedVerilogImmediates(std::ostream &o, const std::size_t immediateProductCount, const std::size_t immediateSumCount) const;
	void printOptimizedVhdlImmediates(std::ostream &o, const std::size_t immediateProductCount, const std::size_t immediateSumCount) const;
	void printOptimizedCppImmediates(std::ostream &o, const std::size_t immediateProductCount, const std::size_t immediateSumCount) const;
	
	void printOptimizedMathArgs(std::ostream &o, const OptimizedSolutions::id_t id) const;
	
	void printOptimizedHumanProduct(std::ostream &o, const OptimizedSolutions::id_t productId) const;
	void printOptimizedGraphProduct(std::ostream &o, const Names &functionNames, const OptimizedSolutions::id_t productId) const;
	void printOptimizedVerilogProduct(std::ostream &o, const OptimizedSolutions::id_t productId) const;
	void printOptimizedVhdlProduct(std::ostream &o, const OptimizedSolutions::id_t productId) const;
	void printOptimizedCppProduct(std::ostream &o, const OptimizedSolutions::id_t productId) const;
	void printOptimizedMathProduct(std::ostream &o, const OptimizedSolutions::id_t productId) const;
	
	void printOptimizedHumanProducts(std::ostream &o) const;
	void printOptimizedGraphProducts(std::ostream &o, const Names &functionNames) const;
	void printOptimizedVerilogProducts(std::ostream &o) const;
	void printOptimizedVhdlProducts(std::ostream &o) const;
	void printOptimizedCppProducts(std::ostream &o) const;
	void printOptimizedMathProducts(std::ostream &o) const;
	
	void printOptimizedHumanSumBody(std::ostream &o, const OptimizedSolutions::id_t sumId) const;
	void printOptimizedGraphSumProducts(std::ostream &o, const OptimizedSolutions::id_t sumId) const;
	void printOptimizedGraphSumParents(std::ostream &o, const OptimizedSolutions::id_t sumId) const;
	void printOptimizedVerilogSumBody(std::ostream &o, const OptimizedSolutions::id_t sumId) const;
	void printOptimizedVhdlSumBody(std::ostream &o, const OptimizedSolutions::id_t sumId) const;
	void printOptimizedCppSumBody(std::ostream &o, const OptimizedSolutions::id_t sumId) const;
	void printOptimizedMathSumBody(std::ostream &o, const OptimizedSolutions::id_t sumId) const;
	
	void printOptimizedHumanSum(std::ostream &o, const OptimizedSolutions::id_t sumId) const;
	void printOptimizedGraphSum(std::ostream &o, const Names &functionNames, const OptimizedSolutions::id_t sumId) const;
	void printOptimizedVerilogSum(std::ostream &o, const OptimizedSolutions::id_t sumId) const;
	void printOptimizedVhdlSum(std::ostream &o, const OptimizedSolutions::id_t sumId) const;
	void printOptimizedCppSum(std::ostream &o, const OptimizedSolutions::id_t sumId) const;
	void printOptimizedMathSum(std::ostream &o, const OptimizedSolutions::id_t sumId) const;
	
	void printOptimizedGraphFinalSum(std::ostream &o, const Names &functionNames, const std::size_t i) const;
	
	void printOptimizedHumanSums(std::ostream &o) const;
	void printOptimizedGraphSums(std::ostream &o, const Names &functionNames) const;
	void printOptimizedVerilogSums(std::ostream &o) const;
	void printOptimizedVhdlSums(std::ostream &o) const;
	void printOptimizedCppSums(std::ostream &o) const;
	void printOptimizedMathSums(std::ostream &o) const;
	
	void printOptimizedHumanFinalSums(std::ostream &o, const Names &functionNames) const;
	void printOptimizedGraphFinalSums(std::ostream &o, const Names &functionNames) const;
	void printOptimizedVerilogFinalSums(std::ostream &o, const Names &functionNames) const;
	void printOptimizedVhdlFinalSums(std::ostream &o, const Names &functionNames) const;
	void printOptimizedCppFinalSums(std::ostream &o, const Names &functionNames) const;
	void printOptimizedMathFinalSums(std::ostream &o, const Names &functionNames) const;
	
	void printOptimizedHumanNegatedInputs(std::ostream &o) const;
	void printOptimizedGraphNegatedInputs(std::ostream &o) const;
	
	void printOptimizedHumanProductBody(std::ostream &o, const OptimizedSolutions::id_t productId) const;
	void printOptimizedVerilogProductBody(std::ostream &o, const OptimizedSolutions::id_t productId) const;
	void printOptimizedVhdlProductBody(std::ostream &o, const OptimizedSolutions::id_t productId) const;
	void printOptimizedCppProductBody(std::ostream &o, const OptimizedSolutions::id_t productId) const;
	void printOptimizedMathProductBody(std::ostream &o, const OptimizedSolutions::id_t productId, const bool parentheses) const;
	
	void printOptimizedHumanSolution_(std::ostream &o, const Names &functionNames);
	void printOptimizedGraphSolution_(std::ostream &o, const Names &functionNames);
	void printOptimizedVerilogSolution_(std::ostream &o, const Names &functionNames);
	void printOptimizedVhdlSolution_(std::ostream &o, const Names &functionNames);
	void printOptimizedCppSolution_(std::ostream &o, const Names &functionNames);
	void printOptimizedMathSolution_(std::ostream &o, const Names &functionNames);
	
	void printOptimizedHumanSolution(std::ostream &o);
	void printOptimizedGraphSolution(std::ostream &o);
	void printOptimizedVerilogSolution(std::ostream &o);
	void printOptimizedVhdlSolution(std::ostream &o, const Names &functionNames);
	void printOptimizedCppSolution(std::ostream &o, const Names &functionNames);
	void printOptimizedMathSolution(std::ostream &o, const Names &functionNames);
	
	void printHuman(std::ostream &o);
	void printGraph(std::ostream &o);
	void printVerilog(std::ostream &o);
	void printVhdl(std::ostream &o);
	void printCpp(std::ostream &o);
	void printMath(std::ostream &o);
	
	void printGateCost(std::ostream &o);
	
public:
	OutputComposer(const Names &functionNames, std::vector<Karnaugh> &karnaughs, const Solutions &solutions, const OptimizedSolutions *const optimizedSolutions) : functionNames(functionNames), karnaughs(karnaughs), solutions(solutions), optimizedSolutions(optimizedSolutions) {}
	
	void compose(std::ostream &o);
};
