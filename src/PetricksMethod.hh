#pragma once

#include <cstddef>
#include <set>
#include <utility>
#include <vector>

#include "Minterm.hh"
#include "PetricksHasseDiagram.hh"
#include "PrimeImplicants.hh"


template<typename INDEX_T>
class PetricksMethod
{
public:
	using minterms_t = std::set<Minterm>;
	using solutions_t = std::vector<PrimeImplicants>;
	
private:
	using index_t = INDEX_T;
	static constexpr index_t NO_INDEX = ~index_t(0);
	using product_t = std::vector<index_t>;
	using sumOfProducts_t = std::vector<product_t>;
	using productOfSumsOfProducts_t = std::vector<sumOfProducts_t>;
	
	minterms_t minterms;
	PrimeImplicants primeImplicants;
	
	PetricksMethod(minterms_t &&minterms, PrimeImplicants &&primeImplicants) : minterms(std::move(minterms)), primeImplicants(std::move(primeImplicants)) {}
	
	index_t findEssentialPrimeImplicantIndex(const Minterm minterm);
	PrimeImplicants extractEssentials();
	productOfSumsOfProducts_t createPreliminaryProductOfSums() const;
	static void removeRedundantSums(productOfSumsOfProducts_t &productOfSums);
	productOfSumsOfProducts_t createProductOfSums() const;
	static sumOfProducts_t multiplySumsOfProducts(const sumOfProducts_t &multiplier0, const sumOfProducts_t &multiplier1);
	sumOfProducts_t findSumOfProducts() const;
	solutions_t solve();
	
public:
	static constexpr std::size_t MAX_PRIME_IMPL_COUNT = PetricksHasseDiagram<index_t>::MAX_VALUE;
	
	static solutions_t solve(minterms_t minterms, PrimeImplicants primeImplicants) { return PetricksMethod(std::move(minterms), std::move(primeImplicants)).solve(); }
};
