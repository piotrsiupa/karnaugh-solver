#pragma once

#include <cstddef>
#include <set>
#include <utility>
#include <vector>

#include "Implicants.hh"
#include "Minterm.hh"
#include "HasseDiagram.hh"
#include "Progress.hh"


template<typename INDEX_T>
class PetricksMethod
{
public:
	using minterms_t = std::set<Minterm>;
	using solutions_t = std::vector<Implicants>;
	
private:
	using index_t = INDEX_T;
	static constexpr index_t NO_INDEX = static_cast<index_t>(~index_t(0));
	using product_t = std::vector<index_t>;
	using sumOfProducts_t = std::vector<product_t>;
	using productOfSumsOfProducts_t = std::vector<sumOfProducts_t>;
	
	minterms_t minterms;
	Implicants primeImplicants;
	
	PetricksMethod(minterms_t &&minterms, Implicants &&primeImplicants) : minterms(std::move(minterms)), primeImplicants(std::move(primeImplicants)) {}
	
	index_t findEssentialPrimeImplicantIndex(const Minterm minterm);
	Implicants extractEssentials();
	productOfSumsOfProducts_t createPreliminaryProductOfSums() const;
	static void removeRedundantSums(productOfSumsOfProducts_t &productOfSums);
	productOfSumsOfProducts_t createProductOfSums() const;
	static sumOfProducts_t multiplySumsOfProducts(const sumOfProducts_t &multiplier0, const sumOfProducts_t &multiplier1, long double &actualOperations, const long double expectedOperations, Progress &progress);
	static std::string ld2integerString(const long double value);
	sumOfProducts_t findSumOfProducts(Progress &progress) const;
	solutions_t solve(Progress &progress);
	
public:
	static constexpr std::size_t MAX_PRIME_IMPL_COUNT = HasseDiagram<index_t>::MAX_VALUE;
	
	static solutions_t solve(minterms_t minterms, Implicants primeImplicants, Progress &progress) { return PetricksMethod(std::move(minterms), std::move(primeImplicants)).solve(progress); }
};
