#pragma once

#include <concepts>
#include <cstddef>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "Minterm.hh"
#include "Minterms.hh"
#include "HasseDiagram.hh"
#include "Implicant.hh"
#include "Progress.hh"
#include "Solution.hh"
#include "Solutions.hh"


template<std::unsigned_integral INDEX_T>
class PetricksMethod
{
public:
	using primeImplicants_t = std::vector<Implicant>;
	
private:
	using index_t = INDEX_T;
	static constexpr index_t NO_INDEX = static_cast<index_t>(~index_t(0));
	using product_t = std::vector<index_t>;
	using sumOfProducts_t = std::vector<product_t>;
	using productOfSumsOfProducts_t = std::vector<sumOfProducts_t>;
	
	std::optional<Minterms> minterms;
	primeImplicants_t primeImplicants;
	
	PetricksMethod(Minterms &&minterms, primeImplicants_t &&primeImplicants) : minterms(std::in_place, std::move(minterms)), primeImplicants(std::move(primeImplicants)) {}
	
	index_t findEssentialPrimeImplicantIndex(const Minterm minterm);
	primeImplicants_t extractEssentials(const std::string &functionName);
	productOfSumsOfProducts_t createPreliminaryProductOfSums(const std::string &functionName) const;
	static void removeRedundantSums(productOfSumsOfProducts_t &productOfSums, const std::string &functionName);
	productOfSumsOfProducts_t createProductOfSums(const std::string &functionName);
	static sumOfProducts_t multiplySumsOfProducts(const sumOfProducts_t &multiplier0, const sumOfProducts_t &multiplier1, long double &actualOperations, const long double expectedOperations, Progress &progress);
	static std::string ld2integerString(const long double value);
	sumOfProducts_t findSumOfProducts(const std::string &functionName);
	Solutions solve(const std::string &functionName);
	
public:
	static constexpr std::size_t MAX_PRIME_IMPL_COUNT = HasseDiagram<index_t>::MAX_VALUE;
	
	static Solutions solve(Minterms minterms, primeImplicants_t primeImplicants, const std::string &functionName) { return PetricksMethod(std::move(minterms), std::move(primeImplicants)).solve(functionName); }
};
