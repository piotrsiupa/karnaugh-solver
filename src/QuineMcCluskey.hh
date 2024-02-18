#pragma once

#include <string>
#include <vector>

#include "Implicant.hh"
#include "Minterms.hh"
#include "PetricksMethod.hh"
#include "Solutions.hh"


class QuineMcCluskey
{
	using primeImplicants_t = typename PetricksMethod<unsigned>::primeImplicants_t;
	
	primeImplicants_t findPrimeImplicants(const Minterms &allowedMinterms, const std::string &functionName) const;
	
public:
	Solutions solve(const Minterms &allowedMinterms, const Minterms &targetMinterms, const std::string &functionName) const;
};
