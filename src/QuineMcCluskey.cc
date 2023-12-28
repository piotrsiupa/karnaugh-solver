#include "./QuineMcCluskey.hh"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>

#include "options.hh"
#include "PetricksMethod.hh"
#include "Progress.hh"
#include "utils.hh"


class Minterator
{
	Minterm minterm;
	bool overflow = false;
	
public:
	constexpr Minterator(const Implicant &implicant) : minterm(implicant.firstMinterm()) {}
	
	[[nodiscard]] constexpr operator bool() const { return !overflow; }
	void step(const Implicant &implicant) { if (minterm == implicant.lastMinterm()) overflow = true; else minterm = implicant.nextMinterm(minterm); }
	[[nodiscard]] constexpr Minterm operator*() const { return minterm; }
};


Implicants QuineMcCluskey::findPrimeImplicants(Minterms allowedMinterms, const std::string &functionName) const
{
	const std::string progressName = "Finding prime impl. of \"" + functionName + '"';
	Progress progress(Progress::Stage::SOLVING, progressName.c_str(), 5);
	
	std::vector<Minterm> masks(std::size_t(::maxMinterm) + 1);
	{
		const auto subtaskGuard = progress.enterSubtask("listing minterm neighbours");
		progress.step();
		Progress::CountingSubsteps substeps = progress.makeCountingSubsteps(allowedMinterms.getSize());
		if (::bits != 0)
		{
			const Minterm maxBit = 1 << (::bits - 1);
			for (const Minterm minterm : allowedMinterms)
			{
				substeps.substep();
				Minterm mask = ::maxMinterm;
				for (Minterm bit = 1;; bit <<= 1)
				{
					if (allowedMinterms.check(minterm ^ bit))
						mask &= ~bit;
					if (bit == maxBit)
						break;
				}
				masks[minterm] = mask;
			}
		}
	}
	
	std::vector<Implicant> newImplicants;
	
	{
		progress.step();
		Progress::CountingSubsteps substeps = progress.makeCountingSubsteps(allowedMinterms.getSize() + (allowedMinterms.getSize() / 5));
		{
			const auto subtaskGuard = progress.enterSubtask("merging implicants with a heuristic");
			for (const Minterm minterm : allowedMinterms)
			{
				substeps.substep();
				Implicant implicant(minterm & masks[minterm], masks[minterm]);
				for (Minterator iter(implicant); iter; iter.step(implicant))
				{
					if (allowedMinterms.check(*iter))
					{
						implicant.add({static_cast<Implicant::mask_t>(*iter & masks[*iter]), static_cast<Implicant::mask_t>(masks[*iter])});
					}
					else
					{
						implicant = Implicant::none();
						break;
					}
				}
				if (implicant != Implicant::none())
					newImplicants.push_back(implicant);
			}
		}
		{
			const auto subtaskGuard = progress.enterSubtask("cleaning up the heuristic (*)");
			substeps.substep(true);
			std::sort(newImplicants.begin(), newImplicants.end());
			newImplicants.erase(std::unique(newImplicants.begin(), newImplicants.end()), newImplicants.end());
			for (const Implicant &newImplicant : newImplicants)
				newImplicant.removeFromMinterms(allowedMinterms);
		}
	}
	
	{
		const auto subtaskGuard = progress.enterSubtask("adding missing implicants");
		progress.step();
		Progress::CountingSubsteps substeps = progress.makeCountingSubsteps(allowedMinterms.getSize());
		newImplicants.reserve(newImplicants.size() + allowedMinterms.getSize());
		for (const Minterm minterm : allowedMinterms)
		{
			substeps.substep();
			newImplicants.push_back(Implicant(minterm));
		}
	}
	
	Implicants primeImplicants;
	{
		progress.step();
		std::vector<Implicant> implicants;
		std::vector<bool> usedImplicants, obsoleteImplicants;
		char subtaskDescription[0x80] = "";
		const auto subtaskGuard = progress.enterSubtask(subtaskDescription);
		::bits_t bitCountLimit;
		std::size_t i;
		Progress::calcSubstepCompletion_t calcSubstepCompletion = [&bitCountLimit = std::as_const(bitCountLimit), &implicants = std::as_const(implicants), &i = std::as_const(i)](){
				const Progress::completion_t majorProgress = static_cast<Progress::completion_t>(::bits - bitCountLimit) / static_cast<Progress::completion_t>(::bits + 1);
				const Progress::completion_t minorProgress = static_cast<Progress::completion_t>(i) / static_cast<Progress::completion_t>(implicants.size()) / static_cast<Progress::completion_t>(::bits + 1);
				return majorProgress + minorProgress;
			};
		for (bitCountLimit = ::bits; !newImplicants.empty() || !implicants.empty(); --bitCountLimit)
		{
			i = 0;
			progress.substep(calcSubstepCompletion, true);
			
			const std::size_t oldImplicantCount = implicants.size();
			std::sort(newImplicants.begin(), newImplicants.end());
			newImplicants.erase(std::unique(newImplicants.begin(), newImplicants.end()), newImplicants.end());
			implicants.reserve(implicants.size() + newImplicants.size());
			std::sort(implicants.begin(), implicants.end());
			implicants.insert(implicants.end(), newImplicants.begin(), newImplicants.end());
			usedImplicants.assign(implicants.size(), false);
			obsoleteImplicants.assign(implicants.size(), false);
			newImplicants.clear();
			const decltype(implicants.begin()) firstNewImplicant = std::next(implicants.begin(), oldImplicantCount);
			
			if (progress.isVisible())
			{
				std::strcpy(subtaskDescription, "merging impl. stage ");
				std::strcat(subtaskDescription, std::to_string(::bits - bitCountLimit + 1).c_str());
				std::strcat(subtaskDescription, "/");
				std::strcat(subtaskDescription, std::to_string(::bits + 1).c_str());
				std::strcat(subtaskDescription, " (");
				std::strcat(subtaskDescription, std::to_string(firstNewImplicant - implicants.cbegin()).c_str());
				std::strcat(subtaskDescription, "+");
				std::strcat(subtaskDescription, std::to_string(implicants.cend() - firstNewImplicant).c_str());
				std::strcat(subtaskDescription, ")");
			}
			
			for (i = 0; i != implicants.size(); ++i)
			{
				progress.substep(calcSubstepCompletion);
				for (std::size_t j = std::max(i + 1, static_cast<std::size_t>(firstNewImplicant - implicants.cbegin())); j != implicants.size(); ++j)
				{
					Implicant newImplicant = Implicant::findBiggestInUnion(implicants[i], implicants[j]);
					if (newImplicant != Implicant::none() && newImplicant.getBitCount() < bitCountLimit)
					{
						usedImplicants[i] = usedImplicants[j] = true;
						if (newImplicant.contains(implicants[i]))
							obsoleteImplicants[i] = true;
						if (newImplicant.contains(implicants[j]))
							obsoleteImplicants[j] = true;
						if (!std::binary_search(implicants.begin(), firstNewImplicant, newImplicant)
								&& !std::binary_search(firstNewImplicant, implicants.end(), newImplicant)
								&& !std::binary_search(primeImplicants.cbegin(), primeImplicants.cend(), newImplicant))
							newImplicants.push_back(std::move(newImplicant));
					}
				}
			}
			
			implicants.erase(remove_if_index(implicants.begin(), implicants.end(), [&primeImplicants, &usedImplicants = std::as_const(usedImplicants), &obsoleteImplicants = std::as_const(obsoleteImplicants)](const Implicant &x, const std::size_t &i)
				{
					if (obsoleteImplicants[i])
						return true;
					if (!usedImplicants[i])
					{
						primeImplicants.push_back(std::move(x));
						return true;
					}
					return false;
				}), implicants.end());
			std::sort(primeImplicants.begin(), primeImplicants.end());
		}
	}
	
	{
		const auto subtaskGuard = progress.enterSubtask("sorting prime implicants (*)");
		progress.step();
		progress.substep([](){ return 0.0; }, true);
		primeImplicants.humanSort();
		primeImplicants.shrink_to_fit();
	}
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
