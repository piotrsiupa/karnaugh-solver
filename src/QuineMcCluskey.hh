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
	
public:
	using solutions_t = std::vector<Implicants>;
	
	solutions_t solve(const Minterms &allowedMinterms, const Minterms &targetMinterms, const std::string &functionName) const;
};
