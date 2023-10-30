#include "./QuineMcCluskey.hh"

#include <cstdint>
#include <iostream>
#include <set>

#include "PetricksMethod.hh"


Implicants QuineMcCluskey::findPrimeImplicants(const Minterms &allowedMinterms) const
{
	std::vector<std::pair<Implicant, bool>> oldImplicants;
	for (const Minterm &minterm : allowedMinterms)
		oldImplicants.emplace_back(Implicant{minterm}, false);
	
	Implicants primeImplicants;
	
	while (!oldImplicants.empty())
	{
		std::set<Implicant> newImplicants;
		
		for (auto iter = oldImplicants.begin(); iter != oldImplicants.end(); ++iter)
		{
			for (auto jiter = std::next(iter); jiter != oldImplicants.end(); ++jiter)
			{
				if (Implicant::areMergeable(iter->first, jiter->first))
				{
					newImplicants.insert(Implicant::merge(iter->first, jiter->first));
					iter->second = true;
					jiter->second = true;
				}
			}
		}
		
		for (const auto &[implicant, merged] : oldImplicants)
			if (!merged)
				primeImplicants.push_back(implicant);
		oldImplicants.clear();
		
		oldImplicants.reserve(newImplicants.size());
		for (const auto &newImplicant : newImplicants)
			oldImplicants.emplace_back(newImplicant, false);
	}
	
	return primeImplicants;
}

QuineMcCluskey::solutions_t QuineMcCluskey::solve(const Minterms &allowedMinterms, const Minterms &targetMinterms, Progress &progress) const
{
	Implicants primeImplicants = findPrimeImplicants(allowedMinterms);
	if (primeImplicants.size() <= PetricksMethod<std::uint8_t>::MAX_PRIME_IMPL_COUNT)
		return PetricksMethod<std::uint8_t>::solve(targetMinterms, std::move(primeImplicants), progress);
	else if (primeImplicants.size() <= PetricksMethod<std::uint16_t>::MAX_PRIME_IMPL_COUNT)
		return PetricksMethod<std::uint16_t>::solve(targetMinterms, std::move(primeImplicants), progress);
	else if (primeImplicants.size() <= PetricksMethod<std::uint32_t>::MAX_PRIME_IMPL_COUNT)
		return PetricksMethod<std::uint32_t>::solve(targetMinterms, std::move(primeImplicants), progress);
	else if (primeImplicants.size() <= PetricksMethod<std::uint64_t>::MAX_PRIME_IMPL_COUNT)
		return PetricksMethod<std::uint64_t>::solve(targetMinterms, std::move(primeImplicants), progress);
	else
		std::cerr << "The number of prime implicants is ridiculous and this has no right to work! I won't even try.\n";
	return {};
}
