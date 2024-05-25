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
#include "utils.hh"


class OutputComposer
{
	using grayCode_t = std::vector<Minterm>;
	
	const Names &functionNames;
	const std::vector<Karnaugh> &karnaughs;
	const Solutions &solutions;
	const OptimizedSolutions *const optimizedSolutions;
	
	[[nodiscard]] static inline bool isHuman();
	[[nodiscard]] static inline bool isGraph();
	[[nodiscard]] static inline bool isHumanReadable();
	
	[[nodiscard]] std::pair<bool, bool> checkForUsedConstants(const Solution &solution) const;
	[[nodiscard]] std::pair<bool, bool> checkForUsedConstants() const;
	[[nodiscard]] bool areInputsUsed() const;
	
	[[nodiscard]] bool isOptimizedProductWorthPrintingInGeneral(const OptimizedSolutions::id_t productId) const;
	[[nodiscard]] bool isOptimizedProductWorthPrinting(const OptimizedSolutions::id_t productId) const;
	[[nodiscard]] bool isOptimizedSumWorthPrintingInGeneral(const OptimizedSolutions::id_t sumId) const;
	[[nodiscard]] bool isOptimizedSumWorthPrinting(const OptimizedSolutions::id_t sumId) const;
	
	[[nodiscard]] static std::string getName();
	
	std::vector<OptimizedSolutions::id_t> normalizedOptimizedIds;
	std::pair<std::size_t, std::size_t> generateOptimizedNormalizedIds();
	void printNormalizedId(std::ostream &o, const OptimizedSolutions::id_t id, const bool useHumanAnyway = false) const;
	
	static void printBanner(std::ostream &o);
	
	static void printShortBool(std::ostream &o, const Trilean value);
	static void printBool(std::ostream &o, const bool value, const bool strictlyForCode = false);
	static void printNot(std::ostream &o);
	static void printAnd(std::ostream &o, const bool spaces);
	static void printOr(std::ostream &o, const bool spaces);
	
	static grayCode_t makeGrayCode(const bits_t bitCount);
	static void printBits(std::ostream &o, const Minterm minterm, const bits_t bitCount);
	static void prettyPrintTable(std::ostream &o, const Minterms &target, const Minterms &allowed = {});
	void prettyPrintTable(std::ostream &o, const std::size_t i) const;
	static void prettyPrintSolution(std::ostream &o, const Solution &solution);
	
	void printImplicant(std::ostream &o, const Implicant &implicant, const bool parentheses, const bool useHumanAnyway = false) const;
	
	void printGateCost(std::ostream &o, const GateCost &gateCost, const bool full) const;
	
	void printGraphInputs(std::ostream &o) const;
	static void printGraphParentBit(std::ostream &o, const std::size_t functionNum, const Implicant::splitBit_t &splitBit, const std::size_t i);
	[[nodiscard]] std::size_t printGraphProducts(std::ostream &o, const Solution &solution, const std::size_t functionNum, std::size_t idShift) const;
	void printGraphSum(std::ostream &o, const Solution &solution, const std::size_t functionNum) const;
	
	std::size_t printSolution(std::ostream &o, const std::size_t i, const std::size_t idShift = 0) const;
	void printSolutions(std::ostream &o) const;
	
	void printOptimizedImmediates(std::ostream &o, const std::size_t immediateProductCount, const std::size_t immediateSumCount) const;
	
	void printOptimizedMathArgs(std::ostream &o, const OptimizedSolutions::id_t id) const;
	
	void printOptimizedNegatedInputs(std::ostream &o) const;
	
	void printOptimizedProductBody(std::ostream &o, const OptimizedSolutions::id_t productId, const bool parentheses) const;
	void printOptimizedGraphProductImplicant(std::ostream &o, const OptimizedSolutions::id_t productId) const;
	std::size_t printOptimizedGraphProductLabel(std::ostream &o, const OptimizedSolutions::id_t productId, const std::size_t functionNum) const;
	void printOptimizedGraphProductParents(std::ostream &o, const OptimizedSolutions::id_t productId) const;
	void printOptimizedProduct(std::ostream &o, const OptimizedSolutions::id_t productId) const;
	void printOptimizedProducts(std::ostream &o) const;
	
	void printOptimizedSumBody(std::ostream &o, const OptimizedSolutions::id_t sumId) const;
	std::size_t printOptimizedGraphSumLabel(std::ostream &o, const OptimizedSolutions::id_t sumId, std::size_t functionNum) const;
	void printOptimizedGraphSumProducts(std::ostream &o, const OptimizedSolutions::id_t sumId) const;
	void printOptimizedGraphSumParents(std::ostream &o, const OptimizedSolutions::id_t sumId) const;
	void printOptimizedSum(std::ostream &o, const OptimizedSolutions::id_t sumId) const;
	void printOptimizedSums(std::ostream &o) const;
	
	void printOptimizedGraphFinalSumLabel(std::ostream &o, const std::size_t i) const;
	void printOptimizedFinalSums(std::ostream &o) const;
	
	void printOptimizedSolution(std::ostream &o);
	
	void printGraphConstants(std::ostream &o) const;
	void printGraphNegatedInputs(std::ostream &o, const Solution &solution, const std::size_t functionNum) const;
	void printGraphRoots(std::ostream &o) const;
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
