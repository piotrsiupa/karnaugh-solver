#pragma once

#include <string>
#include <vector>

#include "Implicants.hh"
#include "Minterm.hh"
#include "Minterms.hh"
#include "Progress.hh"


class QuineMcCluskey
{
	Implicants findPrimeImplicants(Minterms allowedMinterms, const std::string &functionName) const;
	
	static std::vector<Minterm>::iterator mergeSomeMinterms(const std::vector<Minterm>::iterator begin, const std::vector<Minterm>::iterator end, std::vector<Minterm>::iterator remaining, std::vector<Minterm> bits, std::vector<Implicant> &implicants, Progress::CountingStepHelper<std::size_t> &progressStep);
	
public:
	using solutions_t = std::vector<Implicants>;
	
	solutions_t solve(const Minterms &allowedMinterms, const Minterms &targetMinterms, const std::string &functionName) const;
};
