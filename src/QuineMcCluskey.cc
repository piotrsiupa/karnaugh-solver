#include "./QuineMcCluskey.hh"

#include <set>

#include "PetricksMethod.hh"


PrimeImplicants QuineMcCluskey::findPrimeImplicants(const Minterms &allowedMinterms) const
{
	std::vector<std::pair<PrimeImplicant, bool>> oldPrimeImplicants;
	for (const Minterm &minterm : allowedMinterms)
		oldPrimeImplicants.emplace_back(PrimeImplicant{minterm}, false);
	
	PrimeImplicants primeImplicants;
	
	while (!oldPrimeImplicants.empty())
	{
		std::set<PrimeImplicant> newPrimeImplicants;
		
		for (auto iter = oldPrimeImplicants.begin(); iter != oldPrimeImplicants.end(); ++iter)
		{
			for (auto jiter = std::next(iter); jiter != oldPrimeImplicants.end(); ++jiter)
			{
				if (PrimeImplicant::areMergeable(iter->first, jiter->first))
				{
					newPrimeImplicants.insert(PrimeImplicant::merge(iter->first, jiter->first));
					iter->second = true;
					jiter->second = true;
				}
			}
		}
		
		for (const auto &[primeImplicant, merged] : oldPrimeImplicants)
			if (!merged)
				primeImplicants.push_back(primeImplicant);
		oldPrimeImplicants.clear();
		
		oldPrimeImplicants.reserve(newPrimeImplicants.size());
		for (const auto &newPrimeImplicant : newPrimeImplicants)
			oldPrimeImplicants.emplace_back(newPrimeImplicant, false);
	}
	
	return primeImplicants;
}

QuineMcCluskey::solutions_t QuineMcCluskey::solve(const Minterms &allowedMinterms, const Minterms &targetMinterms) const
{
	PrimeImplicants primeImplicants = findPrimeImplicants(allowedMinterms);
	return PetricksMethod<Minterm, PrimeImplicant>::solve(targetMinterms, std::move(primeImplicants));
}
