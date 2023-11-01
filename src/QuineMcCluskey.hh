#pragma once

#include <string>
#include <vector>

#include "Implicants.hh"
#include "Minterms.hh"


class QuineMcCluskey
{
	Implicants findPrimeImplicants(const Minterms &allowedMinterms) const;
	
public:
	using solutions_t = std::vector<Implicants>;
	
	solutions_t solve(const Minterms &allowedMinterms, const Minterms &targetMinterms, const std::string &functionName) const;
};
