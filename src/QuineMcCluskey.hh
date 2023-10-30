#pragma once

#include <vector>

#include "Minterms.hh"
#include "PrimeImplicants.hh"
#include "Progress.hh"


class QuineMcCluskey
{
	PrimeImplicants findPrimeImplicants(const Minterms &allowedMinterms) const;
	
public:
	using solutions_t = std::vector<PrimeImplicants>;
	
	solutions_t solve(const Minterms &allowedMinterms, const Minterms &targetMinterms, Progress &progress) const;
};
