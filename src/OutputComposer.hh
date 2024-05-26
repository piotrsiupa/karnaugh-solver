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
	std::ostream &o;
	
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
	void printNormalizedId(const OptimizedSolutions::id_t id, const bool useHumanAnyway = false) const;
	
	void printCommentStart() const;
	void printAssignmentStart() const;
	void printAssignmentOp() const;
	void printShortBool(const Trilean value) const;
	void printBool(const bool value, const bool strictlyForCode = false) const;
	void printNot() const;
	void printAnd(const bool spaces) const;
	void printOr(const bool spaces) const;
	
	void printBanner() const;
	
	static grayCode_t makeGrayCode(const bits_t bitCount);
	void printBits(const Minterm minterm, const bits_t bitCount) const;
	void prettyPrintTable(const Minterms &target, const Minterms &allowed = {}) const;
	void prettyPrintTable(const std::size_t i) const;
	void prettyPrintSolution(const Solution &solution) const;
	
	void printImplicant(const Implicant &implicant, const bool parentheses, const bool useHumanAnyway = false) const;
	
	void printGateCost(const GateCost &gateCost, const bool full) const;
	
	void printGraphInputs() const;
	void printGraphParentBit(const std::size_t functionNum, const Implicant::splitBit_t &splitBit, const std::size_t i) const;
	[[nodiscard]] std::size_t printGraphProducts(const Solution &solution, const std::size_t functionNum, std::size_t idShift) const;
	void printGraphSum(const Solution &solution, const std::size_t functionNum) const;
	
	std::size_t printSolution(const std::size_t i, const std::size_t idShift = 0) const;
	void printSolutions() const;
	
	void printOptimizedImmediates(const std::size_t immediateProductCount, const std::size_t immediateSumCount) const;
	
	void printOptimizedMathArgs(const OptimizedSolutions::id_t id) const;
	
	void printOptimizedNegatedInputs() const;
	
	void printOptimizedProductBody(const OptimizedSolutions::id_t productId, const bool parentheses) const;
	void printOptimizedGraphProductImplicant(const OptimizedSolutions::id_t productId) const;
	std::size_t printOptimizedGraphProductLabel(const OptimizedSolutions::id_t productId, const std::size_t functionNum) const;
	void printOptimizedGraphProductParents(const OptimizedSolutions::id_t productId) const;
	void printOptimizedProduct(const OptimizedSolutions::id_t productId) const;
	void printOptimizedProducts() const;
	
	void printOptimizedSumBody(const OptimizedSolutions::id_t sumId) const;
	std::size_t printOptimizedGraphSumLabel(const OptimizedSolutions::id_t sumId, std::size_t functionNum) const;
	void printOptimizedGraphSumProducts(const OptimizedSolutions::id_t sumId) const;
	void printOptimizedGraphSumParents(const OptimizedSolutions::id_t sumId) const;
	void printOptimizedSum(const OptimizedSolutions::id_t sumId) const;
	void printOptimizedSums() const;
	
	void printOptimizedGraphFinalSumLabel(const std::size_t i) const;
	void printOptimizedFinalSums() const;
	
	void printOptimizedSolution();
	
	void printGraphConstants() const;
	void printGraphNegatedInputs(const Solution &solution, const std::size_t functionNum) const;
	void printGraphRoots() const;
	void printHuman();
	void printGraph();
	void printVerilog();
	void printVhdl();
	void printCpp();
	void printMath();
	void printGateCost();
	
public:
	OutputComposer(const Names &functionNames, std::vector<Karnaugh> &karnaughs, const Solutions &solutions, const OptimizedSolutions *const optimizedSolutions, std::ostream &o);
	
	void compose();
};
