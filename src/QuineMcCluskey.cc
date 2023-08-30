#include "./QuineMcCluskey.hh"

#include <cstdint>
#include <iostream>
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
	if (primeImplicants.size() <= PetricksMethod<Minterm, PrimeImplicant, std::uint8_t>::MAX_PRIME_IMPL_COUNT)
		return PetricksMethod<Minterm, PrimeImplicant, std::uint8_t>::solve(targetMinterms, std::move(primeImplicants));
	else if (primeImplicants.size() <= PetricksMethod<Minterm, PrimeImplicant, std::uint16_t>::MAX_PRIME_IMPL_COUNT)
		return PetricksMethod<Minterm, PrimeImplicant, std::uint16_t>::solve(targetMinterms, std::move(primeImplicants));
	else if (primeImplicants.size() <= PetricksMethod<Minterm, PrimeImplicant, std::uint32_t>::MAX_PRIME_IMPL_COUNT)
		return PetricksMethod<Minterm, PrimeImplicant, std::uint32_t>::solve(targetMinterms, std::move(primeImplicants));
	else if (primeImplicants.size() <= PetricksMethod<Minterm, PrimeImplicant, std::uint64_t>::MAX_PRIME_IMPL_COUNT)
		return PetricksMethod<Minterm, PrimeImplicant, std::uint64_t>::solve(targetMinterms, std::move(primeImplicants));
	else
		std::cerr << "The number of prime implicants is ridiculous and this has no right to work! I won't even try.\n";
	return {};
}
