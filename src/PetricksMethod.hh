#pragma once

#include <cstddef>
#include <set>
#include <utility>
#include <vector>

#include "PetricksHasseDiagram.hh"


template<typename MINTERM, typename PRIME_IMPLICANT, typename INDEX_T>
class PetricksMethod
{
public:
	using minterm_t = MINTERM;
	using minterms_t = std::set<minterm_t>;
	using primeImplicant_t = PRIME_IMPLICANT;
	using primeImplicants_t = std::vector<primeImplicant_t>;
	using solutions_t = std::vector<primeImplicants_t>;
	
private:
	using index_t = INDEX_T;
	static constexpr index_t NO_INDEX = ~index_t(0);
	using product_t = std::vector<index_t>;
	using sumOfProducts_t = std::vector<product_t>;
	using productOfSumsOfProducts_t = std::vector<sumOfProducts_t>;
	
	minterms_t minterms;
	primeImplicants_t primeImplicants;
	
	PetricksMethod(minterms_t &&minterms, primeImplicants_t &&primeImplicants) : minterms(std::move(minterms)), primeImplicants(std::move(primeImplicants)) {}
	
	index_t findEssentialPrimeImplicantIndex(const minterm_t minterm);
	primeImplicants_t extractEssentials();
	productOfSumsOfProducts_t createPreliminaryProductOfSums() const;
	static void removeRedundantSums(productOfSumsOfProducts_t &productOfSums);
	productOfSumsOfProducts_t createProductOfSums() const;
	static sumOfProducts_t multiplySumsOfProducts(const sumOfProducts_t &multiplier0, const sumOfProducts_t &multiplier1);
	sumOfProducts_t findSumOfProducts() const;
	solutions_t solve();
	
public:
	static constexpr std::size_t MAX_PRIME_IMPL_COUNT = PetricksHasseDiagram<index_t>::MAX_VALUE;
	
	static solutions_t solve(minterms_t minterms, primeImplicants_t primeImplicants) { return PetricksMethod(std::move(minterms), std::move(primeImplicants)).solve(); }
};
