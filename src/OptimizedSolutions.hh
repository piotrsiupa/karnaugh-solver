#pragma once

#include <bit>
#include <cstddef>
#include <set>
#include <utility>
#include <vector>

#include "GateCost.hh"
#include "Implicant.hh"
#include "Progress.hh"
#include "Solution.hh"


class OptimizedSolutions final : public GateCost
{
public:
	using solutions_t = std::vector<const Solution*>;
	
	using id_t = std::size_t;
	using ids_t = std::vector<id_t>;
	using product_t = struct { Implicant implicant; ids_t subProducts; };
	using sum_t = ids_t;
	using finalSums_t = std::vector<id_t>;
	
private:
	using finalPrimeImplicants_t = std::vector<std::size_t>;
	
	Minterm negatedInputs = 0;
	std::vector<product_t> products;
	std::vector<sum_t> sums;
	finalSums_t finalSums;
	
	void createNegatedInputs(const solutions_t &solutions);
	
	finalPrimeImplicants_t extractCommonProductParts(const solutions_t &solutions, Progress &progress);
	void extractCommonSumParts(const solutions_t &solutions, const finalPrimeImplicants_t &finalPrimeImplicants, Progress &progress);
	
#ifndef NDEBUG
	using normalizedSolution_t = std::set<Implicant>;
	normalizedSolution_t normalizeSolution(const id_t finalSumId) const;
	void validate(const solutions_t &solutions, Progress &progress) const;
#endif
	
public:
	OptimizedSolutions() = default;
	OptimizedSolutions(const solutions_t &solutions, Progress &progress);
	
	[[nodiscard]] std::size_t getSize() const { return finalSums.size(); }
	
	[[nodiscard]] std::size_t getProductCount() const { return products.size(); }
	[[nodiscard]] std::size_t getSumCount() const { return sums.size(); }
	[[nodiscard]] id_t getMaxId() const { return static_cast<id_t>(getProductCount()) + static_cast<id_t>(getSumCount()); }
	
	[[nodiscard]] static id_t makeProductId(const std::size_t index) { return index; }
	[[nodiscard]] id_t makeSumId(const std::size_t index) const { return index + products.size(); }
	[[nodiscard]] bool isProduct(const id_t id) const { return id < products.size(); }
	[[nodiscard]] const product_t& getProduct(const id_t id) const { return products[id]; }
	[[nodiscard]] const sum_t& getSum(const id_t id) const { return sums[id - products.size()]; }
	[[nodiscard]] const finalSums_t& getFinalSums() const { return finalSums; }
	
	[[nodiscard]] std::size_t findProductEndNode(const id_t productId, std::size_t startFunctionNum = 0) const;
	[[nodiscard]] std::size_t findSumEndNode(const id_t sumId, const std::size_t startFunctionNum = 0) const;
	[[nodiscard]] std::size_t getIdUseCount(const id_t id) const { return std::accumulate(products.cbegin(), products.cend(), std::size_t(0), [id](const std::size_t &acc, const product_t &product){ return acc + std::count(product.subProducts.cbegin(), product.subProducts.cend(), id); }) + std::accumulate(sums.cbegin(), sums.cend(), std::size_t(0), [id](const std::size_t &acc, const sum_t &sum){ return acc + std::count(sum.cbegin(), sum.cend(), id); }); }
	
	[[nodiscard]] Implicant flattenProduct(const id_t productId) const;
	[[nodiscard]] std::vector<id_t> flattenSum(const id_t sumId) const;
	
	[[nodiscard]] std::size_t getNotCount() const final { return std::popcount(negatedInputs); }
	[[nodiscard]] std::size_t getProductAndCount(const product_t &product) const { return std::max(std::size_t(1), product.implicant.getBitCount() + product.subProducts.size()) - 1; }
	[[nodiscard]] std::size_t getAndCount() const final { std::size_t andCount = 0; for (const auto &product : products) andCount += getProductAndCount(product); return andCount; }
	[[nodiscard]] std::size_t getSumOrCount(const sum_t &sum) const { return sum.size() - 1; }
	[[nodiscard]] std::size_t getOrCount() const final { std::size_t orCount = 0; for (const auto &sum : sums) orCount += getSumOrCount(sum); return orCount; }
	
	[[nodiscard]] const Minterm& getNegatedInputs() const { return negatedInputs; }
	
	[[nodiscard]] std::pair<bool, bool> checkForUsedConstants() const;
};
