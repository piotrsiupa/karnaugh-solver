#pragma once

#include <cstddef>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "Implicants.hh"
#include "Minterm.hh"
#include "Minterms.hh"
#include "HasseDiagram.hh"
#include "Progress.hh"


template<typename INDEX_T>
class PetricksMethod
{
public:
	using solutions_t = std::vector<Implicants>;
	
private:
	using index_t = INDEX_T;
	static constexpr index_t NO_INDEX = static_cast<index_t>(~index_t(0));
	using product_t = std::vector<index_t>;
	using sumOfProducts_t = std::vector<product_t>;
	using productOfSumsOfProducts_t = std::vector<sumOfProducts_t>;
	
	std::optional<Minterms> minterms;
	Implicants primeImplicants;
	
	PetricksMethod(Minterms &&minterms, Implicants &&primeImplicants) : minterms(std::in_place, std::move(minterms)), primeImplicants(std::move(primeImplicants)) {}
	
	index_t findEssentialPrimeImplicantIndex(const Minterm minterm);
	Implicants extractEssentials(const std::string &functionName);
	productOfSumsOfProducts_t createPreliminaryProductOfSums(const std::string &functionName) const;
	static void removeRedundantSums(productOfSumsOfProducts_t &productOfSums, const std::string &functionName);
	productOfSumsOfProducts_t createProductOfSums(const std::string &functionName);
	static sumOfProducts_t multiplySumsOfProducts(const sumOfProducts_t &multiplier0, const sumOfProducts_t &multiplier1, long double &actualOperations, const long double expectedOperations, Progress &progress);
	static std::string ld2integerString(const long double value);
	sumOfProducts_t findSumOfProducts(const std::string &functionName);
	solutions_t solve(const std::string &functionName);
	
public:
	static constexpr std::size_t MAX_PRIME_IMPL_COUNT = HasseDiagram<index_t>::MAX_VALUE;
	
	static solutions_t solve(Minterms minterms, Implicants primeImplicants, const std::string &functionName) { return PetricksMethod(std::move(minterms), std::move(primeImplicants)).solve(functionName); }
};
