#include "./QuineMcCluskey.hh"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <set>
#include <string>

#include "options.hh"
#include "PetricksMethod.hh"
#include "Progress.hh"


Implicants QuineMcCluskey::findPrimeImplicants(const Minterms &allowedMinterms, const std::string &functionName) const
{
	const std::string progressName = "Merging implicants of \"" + functionName + '"';
	Progress progress(Progress::Stage::SOLVING, progressName.c_str(), ::bits + 1);
	
	std::vector<std::pair<Implicant, bool>> implicants;
	for (const Minterm &minterm : allowedMinterms)
		implicants.emplace_back(Implicant{minterm}, false);
	
	Implicants primeImplicants;
	
	::bits_t implicantSize = ::bits;
	char subtaskDescription[96] = "";
	const auto subtaskGuard = progress.enterSubtask(subtaskDescription);
	while (!implicants.empty())
	{
		if (progress.isVisible())
		{
			std::strcpy(subtaskDescription, std::to_string(implicants.size()).c_str());
			std::strcat(subtaskDescription, " left (");
			std::strcat(subtaskDescription, std::to_string(implicantSize--).c_str());
			std::strcat(subtaskDescription, " literals each)");
		}
		Progress::CountingSubsteps substeps = progress.makeCountingSubsteps(::bits * 2);
		progress.step(true);
		
		std::set<Implicant> newImplicants;
		for (::bits_t bit = 0; bit != ::bits; ++bit)
		{
			substeps.substep();
			const Minterm mask = ~(Minterm(1) << bit);
			std::sort(implicants.begin(), implicants.end(), [mask](const std::pair<Implicant, bool> &x, const std::pair<Implicant, bool> &y){
					const Minterm xm = x.first.getRawMask(), ym = y.first.getRawMask();
					if (xm != ym)
						return xm < ym;
					const Minterm xmb = x.first.getRawBits() & mask, ymb = y.first.getRawBits() & mask;
					return xmb < ymb;
				});
			substeps.substep();
			std::pair<Implicant, bool> *previous = &implicants.front();
			for (auto iter = std::next(implicants.begin()); iter != implicants.end(); ++iter)
			{
				const bool maskMakesThemEqual = previous->first.getRawMask() == iter->first.getRawMask()
						&& (previous->first.getRawBits() & mask) == (iter->first.getRawBits() & mask);
				if (maskMakesThemEqual)
				{
					Implicant newImplicant = previous->first;
					newImplicant.applyMask(mask);
					newImplicants.insert(std::move(newImplicant));
					previous->second = true;
					iter->second = true;
					if (++iter == implicants.end())
						break;
				}
				previous = &*iter;
			}
		}
		
		for (const auto &[implicant, merged] : implicants)
			if (!merged)
				primeImplicants.push_back(implicant);
		implicants.clear();
		
		implicants.reserve(newImplicants.size());
		for (const auto &newImplicant : newImplicants)
			implicants.emplace_back(newImplicant, false);
	}
	
	primeImplicants.humanSort();
	return primeImplicants;
}

QuineMcCluskey::solutions_t QuineMcCluskey::solve(const Minterms &allowedMinterms, const Minterms &targetMinterms, const std::string &functionName) const
{
	Implicants primeImplicants = findPrimeImplicants(allowedMinterms, functionName);
	if (primeImplicants.size() <= PetricksMethod<std::uint8_t>::MAX_PRIME_IMPL_COUNT)
		return PetricksMethod<std::uint8_t>::solve(targetMinterms, std::move(primeImplicants), functionName);
	else if (primeImplicants.size() <= PetricksMethod<std::uint16_t>::MAX_PRIME_IMPL_COUNT)
		return PetricksMethod<std::uint16_t>::solve(targetMinterms, std::move(primeImplicants), functionName);
	else if (primeImplicants.size() <= PetricksMethod<std::uint32_t>::MAX_PRIME_IMPL_COUNT)
		return PetricksMethod<std::uint32_t>::solve(targetMinterms, std::move(primeImplicants), functionName);
	else if (primeImplicants.size() <= PetricksMethod<std::uint64_t>::MAX_PRIME_IMPL_COUNT)
		return PetricksMethod<std::uint64_t>::solve(targetMinterms, std::move(primeImplicants), functionName);
	else
		std::cerr << "The number of prime implicants is ridiculous and this has no right to work! I won't even try.\n";
	return {};
}
