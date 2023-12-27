#pragma once

#include <string>
#include <vector>

#include "Implicants.hh"
#include "Minterms.hh"


class QuineMcCluskey
{
	Implicants findPrimeImplicants(Minterms allowedMinterms, const std::string &functionName) const;
	
public:
	using solutions_t = std::vector<Implicants>;
	
	solutions_t solve(const Minterms &allowedMinterms, const Minterms &targetMinterms, const std::string &functionName) const;
};
