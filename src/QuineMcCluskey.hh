#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "Implicants.hh"
#include "Minterm.hh"
#include "Minterms.hh"
#include "Progress.hh"


class QuineMcCluskey
{
public:
	using solutions_t = std::vector<Implicants>;
	
private:
	static std::vector<Minterm> bitMasks;
	
	const std::string &functionName;
	std::shared_ptr<const Minterms> allowedMinterms, targetMinterms;
	
	static void makeBitMasks();
	
	void removeDontCareOnlyImplicants(Implicants &implicants, Progress &progress) const;
	static void cleanupImplicants(Implicants &implicants, Progress &progress);
	
	static inline std::uint_fast8_t getAdjustmentPasses();
	inline void refineHeuristicImplicant(const Minterm initialMinterm, Implicant &implicant) const;
	Implicants createImplicantsWithHeuristic(Progress &progress) const;
	Implicants findPrimeImplicantsWithHeuristic();
	
	Implicants createPrimeImplicantsWithoutHeuristic(Progress &progress);
	Implicants findPrimeImplicantsWithoutHeuristic();
	
	Implicants findPrimeImplicants();
	
	static void validate(const Minterms &allowedMinterms, const Minterms &targetMinterms, const Implicants &implicants);
	
	solutions_t runPetricksMethod(Implicants &&primeImplicants);
	
public:
	QuineMcCluskey(const std::string &functionName, std::shared_ptr<const Minterms> allowedMinterms, std::shared_ptr<const Minterms> targetMinterms) : functionName(functionName), allowedMinterms(std::move(allowedMinterms)), targetMinterms(std::move(targetMinterms)) { if (bitMasks.empty()) makeBitMasks(); }
	
	solutions_t solve() &&;
};
