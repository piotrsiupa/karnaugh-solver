#pragma once

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
	static std::vector<Minterm> listBits();
	
	static Implicants createInitialImplicants(const Minterms &minterms, Progress &progress);
	static void mergeImplicants(Implicants &implicants, Progress &progress);
	static void mergeAndExtendImplicants(const Minterms &minterms, Implicants &implicants, Progress &progress);
	static void createAlternativeImplicants(Implicants &implicants, Progress &progress);
	static void cleanupImplicants(Implicants &implicants, Progress &progress);
	static Implicants findPrimeImplicants(const Minterms &minterms, const std::string &functionName);
	
	static void validate(const Minterms &allowedMinterms, Minterms targetMinterms, const Implicants &implicants);
	
	static solutions_t runPetricksMethod(Implicants &&primeImplicants, const Minterms &targetMinterms, const std::string &functionName);
	
public:
	solutions_t solve(const Minterms &allowedMinterms, const Minterms &targetMinterms, const std::string &functionName) const;
};
