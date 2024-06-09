#pragma once

#include <cstddef>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

#include "GateCost.hh"
#include "IndentedOStream.hh"
#include "Karnaugh.hh"
#include "Names.hh"
#include "OptimizedSolutions.hh"
#include "options.hh"
#include "Solution.hh"
#include "Solutions.hh"
#include "utils.hh"


class OutputComposer
{
	using grayCode_t = std::vector<Minterm>;
	
	const Names functionNames;
	const std::vector<Karnaugh> &karnaughs;
	const Solutions &solutions;
	const OptimizedSolutions *const optimizedSolutions;
	
	options::OutputFormat format;
	options::OutputOperators operators;
	std::string_view name;
	bool isGraphVerbose;
	const options::FilterSpec::Filter *filter;
	
	mutable IndentedOStream o;
	
	// Return `true` when the output format is one of those listed in the template parameters.
	template<options::OutputFormat ...FORMATS>
	[[nodiscard]] static inline constexpr bool discriminate(const options::OutputFormat format);
	template<options::OutputFormat ...FORMATS>
	[[nodiscard]] inline bool discriminate() const;
	
	[[nodiscard]] inline bool isHuman() const;
	[[nodiscard]] inline bool isGraph() const;
	[[nodiscard]] inline bool isHumanReadable() const;
	[[nodiscard]] inline bool isProgramming() const;
	
	// OK, this is quite simple ;-) but maybe some explanation could still be useful.
	// These functions take lists of strings / lambdas / anything else as arguments.
	// Such list corresponds to output formats / groups of output formats and are executed or streamed to `o` when the corresponding format is chosen.
	// (So they works analogous to a `switch` statements. In fact they replace `switch` statements and their purpose is to reduce amount of boiler plate.)
	// There are also 2 special "keywords": `BLANK` that makes the given position do nothing and `NEXT` that works like `fallthrough` in `switch`.
	// `printChoiceSpecific` is a generic implementation that works with any sorted list of numbers. The remaining ones are 6 variants for `options::OutputFormat`.
	// Each of them has 2 versions: one that takes output format as an argument and one that uses `options::outputFormat`.
	// (See the template parameters to check what formats / groups of formats they handle.)
	// So e.g. `printFormatSpecific(NEXT, ';', []{ ... });` will print `;` for `CPP` and `VERILOG`, and it will run the lambda for `VHDL`.
	// `printFormatSpecific(BLANK, " }");` will do nothing for graph formats and print ` }` for "human" formats.
	// (It is an undefined behavior when the current format is not on the list.)
	enum { BLANK };
	static constexpr std::nullptr_t NEXT = nullptr;
	template<typename ENUM, ENUM ...CHOICES, typename ...VALUES>
	inline void printChoiceSpecific(const ENUM choice, const VALUES &...values) const;
	template<typename GRAPH, typename REDUCED_GRAPH, typename MATHEMATICAL, typename GATE_COST, typename HUMAN_LONG, typename HUMAN, typename HUMAN_SHORT, typename CPP, typename VERILOG, typename VHDL>
	inline void printFormatSpecific(const options::OutputFormat format, GRAPH graph, REDUCED_GRAPH reducedGraph, MATHEMATICAL mathematical, GATE_COST gateCost, HUMAN_LONG humanLong, HUMAN human, HUMAN_SHORT humanShort, CPP cpp, VERILOG verilog, VHDL vhdl) const;
	template<typename GRAPH, typename MATHEMATICAL, typename GATE_COST, typename HUMAN, typename CPP, typename VERILOG, typename VHDL>
	inline void printFormatSpecific(const options::OutputFormat format, GRAPH graph, MATHEMATICAL mathematical, GATE_COST gateCost, HUMAN human, CPP cpp, VERILOG verilog, VHDL vhdl) const;
	template<typename GRAPH, typename MATHEMATICAL, typename HUMAN, typename CPP, typename VERILOG, typename VHDL>
	inline void printFormatSpecific(const options::OutputFormat format, GRAPH graph, MATHEMATICAL mathematical, HUMAN human, CPP cpp, VERILOG verilog, VHDL vhdl) const;
	template<typename GRAPH, typename MATHEMATICAL, typename HUMAN, typename PROGRAM>
	inline void printFormatSpecific(const options::OutputFormat format, GRAPH graph, MATHEMATICAL mathematical, HUMAN human, PROGRAM program) const;
	template<typename CPP, typename VERILOG, typename VHDL>
	inline void printFormatSpecific(const options::OutputFormat format, CPP cpp, VERILOG verilog, VHDL vhdl) const;
	template<typename GRAPH, typename HUMAN>
	inline void printFormatSpecific(const options::OutputFormat format, GRAPH graph, HUMAN human) const;
	template<typename GRAPH, typename REDUCED_GRAPH, typename MATHEMATICAL, typename GATE_COST, typename HUMAN_LONG, typename HUMAN, typename HUMAN_SHORT, typename CPP, typename VERILOG, typename VHDL>
	inline std::enable_if_t<!std::is_same_v<GRAPH, options::MappedOutputFormats>, void>
	printFormatSpecific(GRAPH graph, REDUCED_GRAPH reducedGraph, MATHEMATICAL mathematical, GATE_COST gateCost, HUMAN_LONG humanLong, HUMAN human, HUMAN_SHORT humanShort, CPP cpp, VERILOG verilog, VHDL vhdl) const;
	template<typename GRAPH, typename MATHEMATICAL, typename GATE_COST, typename HUMAN, typename CPP, typename VERILOG, typename VHDL>
	inline std::enable_if_t<!std::is_same_v<GRAPH, options::MappedOutputFormats>, void>
	printFormatSpecific(GRAPH graph, MATHEMATICAL mathematical, GATE_COST gateCost, HUMAN human, CPP cpp, VERILOG verilog, VHDL vhdl) const;
	template<typename GRAPH, typename MATHEMATICAL, typename HUMAN, typename CPP, typename VERILOG, typename VHDL>
	inline std::enable_if_t<!std::is_same_v<GRAPH, options::MappedOutputFormats>, void>
	printFormatSpecific(GRAPH graph, MATHEMATICAL mathematical, HUMAN human, CPP cpp, VERILOG verilog, VHDL vhdl) const;
	template<typename GRAPH, typename MATHEMATICAL, typename HUMAN, typename PROGRAM>
	inline std::enable_if_t<!std::is_same_v<GRAPH, options::MappedOutputFormats>, void>
	printFormatSpecific(GRAPH graph, MATHEMATICAL mathematical, HUMAN human, PROGRAM program) const;
	template<typename CPP, typename VERILOG, typename VHDL>
	inline std::enable_if_t<!std::is_same_v<CPP, options::MappedOutputFormats>, void>
	printFormatSpecific(CPP cpp, VERILOG verilog, VHDL vhdl) const;
	template<typename GRAPH, typename HUMAN>
	inline std::enable_if_t<!std::is_same_v<GRAPH, options::MappedOutputFormats>, void>
	printFormatSpecific(GRAPH graph, HUMAN human) const;
	
	[[nodiscard]] Minterm getNegatedInputs() const;
	[[nodiscard]] std::pair<bool, bool> checkForUsedConstants(const Solution &solution) const;
	[[nodiscard]] std::pair<bool, bool> checkForUsedConstants() const;
	[[nodiscard]] bool areInputsUsed() const;
	
	[[nodiscard]] std::size_t findProductFunctionNum(const OptimizedSolutions::id_t productId, const std::size_t startAt = 0) const;
	[[nodiscard]] std::size_t findSumFunctionNum(const OptimizedSolutions::id_t sumId, const std::size_t startAt = 0) const;
	
	[[nodiscard]] bool isOptimizedProductWorthPrintingInGeneral(const OptimizedSolutions::id_t productId) const;
	[[nodiscard]] bool isOptimizedProductWorthPrinting(const OptimizedSolutions::id_t productId) const;
	[[nodiscard]] bool isOptimizedSumWorthPrintingInGeneral(const OptimizedSolutions::id_t sumId) const;
	[[nodiscard]] bool isOptimizedSumWorthPrinting(const OptimizedSolutions::id_t sumId) const;
	[[nodiscard]] bool isOptimizedElementWorthPrinting(const OptimizedSolutions::id_t id) const { return optimizedSolutions->isProduct(id) ? isOptimizedProductWorthPrinting(id) : isOptimizedSumWorthPrinting(id); }
	
	std::vector<bool> visibleElements;
	void generateVisibleElements();
	
	std::vector<OptimizedSolutions::id_t> normalizedOptimizedIds;
	std::pair<std::size_t, std::size_t> generateOptimizedNormalizedIds();
	void printNormalizedId(const OptimizedSolutions::id_t id, const bool useHumanAnyway = false) const;
	
	void printCommentStart() const;
	void printAssignmentStart() const;
	void printAssignmentOp() const;
	void printShortBool(const Trilean value) const;
	[[nodiscard]] static inline constexpr options::OutputFormat mapOutputOperators(const options::OutputOperators operators);
	[[nodiscard]] inline options::OutputFormat getOperatorsStyle() const;
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
	void printGraphProducts(const Solution &solution, const std::size_t functionNum, std::size_t idShift) const;
	void printGraphSum(const Solution &solution, const std::size_t functionNum) const;
	
	void printSolutionsGateCost(const bool full) const;
	
	void printSolution(const std::size_t i, const std::size_t idShift = 0) const;
	void printSolutions() const;
	
	void printOptimizedImmediates(const std::size_t immediateProductCount, const std::size_t immediateSumCount) const;
	
	void printOptimizedMathArgs(const OptimizedSolutions::id_t id) const;
	
	void printOptimizedNegatedInputs() const;
	
	void printOptimizedProductBody(const OptimizedSolutions::id_t productId, const bool parentheses) const;
	void printOptimizedGraphProductImplicant(const OptimizedSolutions::id_t productId) const;
	void printOptimizedGraphProductLabel(const OptimizedSolutions::id_t productId, const std::size_t functionNum) const;
	void printOptimizedGraphProductParents(const OptimizedSolutions::id_t productId) const;
	void printOptimizedProduct(const OptimizedSolutions::id_t productId) const;
	void printOptimizedProducts() const;
	
	void printOptimizedSumBody(const OptimizedSolutions::id_t sumId) const;
	void printOptimizedGraphSumLabel(const OptimizedSolutions::id_t sumId, std::size_t functionNum) const;
	void printOptimizedGraphSumProducts(const OptimizedSolutions::id_t sumId) const;
	void printOptimizedGraphSumParents(const OptimizedSolutions::id_t sumId) const;
	void printOptimizedSum(const OptimizedSolutions::id_t sumId) const;
	void printOptimizedSums() const;
	
	void printOptimizedGraphFinalSumLabel(const std::size_t i) const;
	void printOptimizedFinalSums() const;
	
	void printOptimizedGateCost(const bool full) const;
	
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
	[[nodiscard]] static std::string getStandardName();
	
	OutputComposer(Names &&functionNames, std::vector<Karnaugh> &karnaughs, const Solutions &solutions, const OptimizedSolutions *const optimizedSolutions);
	
	void print(std::ostream &stream, const bool includeBanner, const options::OutputFormat outputFormat, const options::OutputOperators outputOperators, const std::string_view generalName, const bool isGraphVerbose, const options::FilterSpec::Filter &printFilter);
};