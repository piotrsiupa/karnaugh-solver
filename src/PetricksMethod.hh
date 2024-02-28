#pragma once

#include <concepts>
#include <cstddef>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "CompactSet.hh"
#include "Implicants.hh"
#include "Minterm.hh"
#include "Minterms.hh"
#include "Progress.hh"
#include "Solution.hh"
#include "Solutions.hh"


template<std::unsigned_integral INDEX_T>
class PetricksMethod
{
	using index_t = INDEX_T;
	static constexpr index_t NO_INDEX = static_cast<index_t>(~index_t(0));
	using product_t = std::vector<index_t>;
	using sum_t = std::vector<index_t>;
	using sumOfProducts_t = std::vector<product_t>;
	using productOfSums_t = std::vector<product_t>;
	using productOfSumsOfProducts_t = std::vector<sumOfProducts_t>;
	
	std::optional<Minterms> minterms;
	Implicants primeImplicants;
	
	Implicants extractEssentials(const std::string &functionName);
	productOfSums_t createPreliminaryProductOfSums(const std::string &functionName) const;
	static void removeRedundantSums(productOfSums_t &productOfSums, const std::string &functionName);
	productOfSumsOfProducts_t createProductOfSums(const std::string &functionName);
	static std::size_t calcMaxSums();
	static sumOfProducts_t multiplySumsOfProducts(sumOfProducts_t &&multiplier0, const sumOfProducts_t &multiplier1, Progress &progress);
	void multiplySumsOfProducts_maxN(std::vector<CompactSet<index_t>> &multiplier0, const sumOfProducts_t &multiplier1, const std::size_t maxSums);
	static void multiplySumsOfProducts_max1(CompactSet<index_t> &product0, const sumOfProducts_t &multiplier1);
	sumOfProducts_t findSumOfProducts(const std::string &functionName);
	
public:
	PetricksMethod(Minterms minterms, Implicants primeImplicants) : minterms(std::in_place, std::move(minterms)), primeImplicants(std::move(primeImplicants)) {}
	
	Solutions solve(const std::string &functionName) &&;  // This function is `&&` as a reminder the it removes some data in the process (to save memory) and because of that it cannot be called twice.
};
