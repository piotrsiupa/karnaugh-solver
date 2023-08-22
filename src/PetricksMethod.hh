#pragma once

#include <cstddef>
#include <set>
#include <utility>
#include <vector>


template<typename MINTERM, typename PRIME_IMPLICANT>
class PetricksMethod
{
public:
	using minterm_t = MINTERM;
	using minterms_t = std::set<minterm_t>;
	using primeImplicant_t = PRIME_IMPLICANT;
	using primeImplicants_t = std::vector<primeImplicant_t>;
	using solutions_t = std::vector<primeImplicants_t>;
	
private:
	using product_t = std::set<std::size_t>;
	using sumOfProducts_t = std::vector<product_t>;
	using productOfSumsOfProducts_t = std::vector<sumOfProducts_t>;
	
	minterms_t minterms;
	primeImplicants_t primeImplicants;
	
	PetricksMethod(minterms_t &&minterms, primeImplicants_t &&primeImplicants) : minterms(std::move(minterms)), primeImplicants(std::move(primeImplicants)) {}
	
	std::size_t findEssentialPrimeImplicantIndex(const minterm_t minterm);
	primeImplicants_t extractEssentials();
	productOfSumsOfProducts_t createPreliminaryProductOfSums() const;
	static void removeRedundantSums(productOfSumsOfProducts_t &productOfSums);
	productOfSumsOfProducts_t createProductOfSums() const;
	static void removeRedundantProducts(sumOfProducts_t &sumOfProducts);
	static sumOfProducts_t multiplySumsOfProducts(sumOfProducts_t multiplier0, sumOfProducts_t multiplier1);
	sumOfProducts_t findSumOfProducts() const;
	solutions_t solve();
	
public:
	static solutions_t solve(minterms_t minterms, primeImplicants_t primeImplicants) { return PetricksMethod(std::move(minterms), std::move(primeImplicants)).solve(); }
};
