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
	
	static void removeDontCareOnlyImplicants(const Minterms &targetMinterms, Implicants &implicants, Progress &progress);
	static void cleanupImplicants(Implicants &implicants, Progress &progress);
	
	static Implicants createImplicantsWithHeuristic(const Minterms &allowedMinterms, const Minterms &targetMinterms, Progress &progress);
	static Implicants findPrimeImplicantsWithHeuristic(const Minterms &allowedMinterms, const Minterms &targetMinterms, const std::string &functionName);
	
	static Implicants createPrimeImplicantsWithoutHeuristic(const Minterms &allowedMinterms, Progress &progress);
	static Implicants findPrimeImplicantsWithoutHeuristic(const Minterms &allowedMinterms, const Minterms &targetMinterms, const std::string &functionName);
	
	static Implicants findPrimeImplicants(const Minterms &allowedMinterms, const Minterms &targetMinterms, const std::string &functionName);
	
	static void validate(const Minterms &allowedMinterms, Minterms targetMinterms, const Implicants &implicants);
	
	static solutions_t runPetricksMethod(Implicants &&primeImplicants, const Minterms &targetMinterms, const std::string &functionName);
	
public:
	solutions_t solve(const Minterms &allowedMinterms, const Minterms &targetMinterms, const std::string &functionName) const;
};
