#include "./QuineMcCluskey.hh"

#include <cstdint>
#include <cstring>
#include <iostream>
#include <set>
#include <string>

#include "options.hh"
#include "Progress.hh"


QuineMcCluskey::primeImplicants_t QuineMcCluskey::findPrimeImplicants(const Minterms &allowedMinterms, const std::string &functionName) const
{
	const std::string progressName = "Merging implicants of \"" + functionName + '"';
	Progress progress(Progress::Stage::SOLVING, progressName.c_str(), ::bits + 1, true);
	
	std::vector<std::pair<Implicant, bool>> implicants;
	for (const Minterm &minterm : allowedMinterms)
		implicants.emplace_back(Implicant{minterm}, false);
	
	primeImplicants_t primeImplicants;
	
	::bits_t implicantSize = ::bits;
	char subtaskDescription[96] = "";
	const auto infoGuard = progress.addInfo(subtaskDescription);
	while (!implicants.empty())
	{
		std::uintmax_t operationsSoFar = 0, expectedOperations = 0;
		if (progress.isVisible())
		{
			std::strcpy(subtaskDescription, std::to_string(implicants.size()).c_str());
			std::strcat(subtaskDescription, " left (");
			std::strcat(subtaskDescription, std::to_string(implicantSize--).c_str());
			std::strcat(subtaskDescription, " literals each)");
			expectedOperations = static_cast<std::uintmax_t>(implicants.size()) * static_cast<std::uintmax_t>(implicants.size() - 1) / 2;
		}
		const Progress::calcStepCompletion_t calcStepCompletion = [&operationsSoFar = std::as_const(operationsSoFar), expectedOperations](){ return static_cast<Progress::completion_t>(operationsSoFar) / static_cast<Progress::completion_t>(expectedOperations); };
		progress.step(true);
		
		std::set<Implicant> newImplicants;
		
		for (auto iter = implicants.begin(); iter != implicants.end(); ++iter)
		{
			progress.substep(calcStepCompletion);
			operationsSoFar += implicants.cend() - iter - 1;
			for (auto jiter = std::next(iter); jiter != implicants.end(); ++jiter)
			{
				if (Implicant::areMergeable(iter->first, jiter->first))
				{
					newImplicants.insert(Implicant::merge(iter->first, jiter->first));
					iter->second = true;
					jiter->second = true;
				}
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
	
	return primeImplicants;
}

Solutions QuineMcCluskey::solve(const Minterms &allowedMinterms, const Minterms &targetMinterms, const std::string &functionName) const
{
	primeImplicants_t primeImplicants = findPrimeImplicants(allowedMinterms, functionName);
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
