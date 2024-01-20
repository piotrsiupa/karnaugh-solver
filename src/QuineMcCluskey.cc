#include "./QuineMcCluskey.hh"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <map>

#include "options.hh"
#include "PetricksMethod.hh"
#include "utils.hh"


std::vector<Minterm> QuineMcCluskey::listBits()
{
	if (::bits == 0)
		return {};
	
	std::vector<Minterm> bits;
	const Minterm maxBit = Minterm(1) << (::bits - 1);
	bits.reserve(::bits);
	for (Minterm bit = 1;; bit <<= 1)
	{
		bits.push_back(bit);
		if (bit == maxBit)
			break;
	}
	return bits;
}

void QuineMcCluskey::removeDontCareOnlyImplicants(const Minterms &targetMinterms, Implicants &implicants, Progress &progress)
{
	const auto infoGuard = progress.addInfo("removing useless prime implicants");
	progress.step();
	progress.substep([](){ return -0.0; }, true);
	
	implicants.erase(std::remove_if(implicants.begin(), implicants.end(), [&targetMinterms](const Implicant &implicant){ return !implicant.isAnyInMinterms(targetMinterms); }), implicants.end());
}

void QuineMcCluskey::cleanupImplicants(Implicants &implicants, Progress &progress)
{
	const auto infoGuard = progress.addInfo("sorting prime implicants");
	progress.step();
	progress.substep([](){ return -0.0; }, true);
	
	implicants.humanSort();
	implicants.shrink_to_fit();
}

std::uint_fast8_t QuineMcCluskey::getAdjustmentPasses()
{
	if (::bits < 2)
		return 0;
	if (const std::int_fast8_t adjustmentPasses = options::greedyImplicantAdjustments.getValue(); adjustmentPasses >= 0)
		return adjustmentPasses;
	if (::bits < 24)
		return 5;
	else if (::bits == 24)
		return 4;
	else if (::bits == 25)
		return 1;
	else
		return 0;
}

void QuineMcCluskey::refineHeuristicImplicant(const Minterms &allowedMinterms, const std::vector<Minterm> &bits, const Minterm initialMinterm, Implicant &implicant)
{
	for (auto iter = bits.begin(); iter != std::prev(bits.end()); ++iter)
	{
		const Minterm removedBit = *iter;
		if ((removedBit & implicant.getMask()) != 0)
			continue;
		Implicant implicantVariant(implicant.getBits() | (initialMinterm & removedBit), implicant.getMask() | removedBit);
		for (auto jiter = std::next(iter); jiter != bits.end(); ++jiter)
		{
			const Minterm bit = *jiter;
			if ((bit & implicantVariant.getMask()) == 0)
				continue;
			if (Implicant(implicantVariant.getBits() ^ bit, implicantVariant.getMask()).areAllInMinterms(allowedMinterms))
				implicantVariant.applyMask(~bit);
		}
		if (implicantVariant.getBitCount() <= implicant.getBitCount())
			implicant = std::move(implicantVariant);
	}
}

Implicants QuineMcCluskey::createImplicantsWithHeuristic(const Minterms &allowedMinterms, const Minterms &targetMinterms, Progress &progress)
{
	const std::uint_fast8_t adjustmentPasses = getAdjustmentPasses();
	
	const std::vector<Minterm> bits = listBits();
	
	const auto infoGuard = progress.addInfo("Creating implicants with a heuristic");
	progress.step();
	auto progressStep = progress.makeCountingStepHelper(targetMinterms.getSize());
	
	Minterms touchedMinterms;
	Implicants implicants;
	for (const Minterm initialMinterm : targetMinterms)
	{
		progressStep.substep();
		if (touchedMinterms.check(initialMinterm))
			continue;
		Implicant implicant(initialMinterm);
		for (const Minterm bit : bits)
		{
			if (Implicant(implicant.getBits() ^ bit, implicant.getMask()).areAllInMinterms(allowedMinterms))
				implicant.applyMask(~bit);
		}
		for (std::uint_fast64_t i = 0; i != adjustmentPasses; ++i)
			refineHeuristicImplicant(allowedMinterms, bits, initialMinterm, implicant);
		implicant.addToMinterms(touchedMinterms);
		implicants.push_back(std::move(implicant));
	}
	return implicants;
}

Implicants QuineMcCluskey::findPrimeImplicantsWithHeuristic(const Minterms &allowedMinterms, const Minterms &targetMinterms, const std::string &functionName)
{
	if (allowedMinterms.getSize() == 0)
		return {Implicant::none()};
	else if (allowedMinterms.getSize() - 1 == ::maxMinterm)
		return {Implicant::all()};
	
	const std::string progressName = "Finding prime impl. of \"" + functionName + '"';
	Progress progress(Progress::Stage::SOLVING, progressName.c_str(), 2, false);
	
	Implicants implicants = createImplicantsWithHeuristic(allowedMinterms, targetMinterms, progress);
	cleanupImplicants(implicants, progress);
	return implicants;
}

Implicants QuineMcCluskey::createPrimeImplicantsWithoutHeuristic(const Minterms &allowedMinterms, Progress &progress)
{
	Progress::CountingStepHelper step = progress.makeCountingStepHelper(static_cast<std::size_t>(::bits + 1) * static_cast<std::size_t>(::bits));
	progress.step();
	
	std::vector<std::pair<Implicant, bool>> implicants;
	{
		const auto infoGuard = progress.addInfo("preparing initial list of implicants");
		progress.substep([](){ return -0.0; }, true);
		for (const Minterm &minterm : allowedMinterms)
			implicants.emplace_back(Implicant{minterm}, false);
	}
	
	::bits_t implicantSize = ::bits;
	char subtaskDescription[96] = "";
	const auto infoGuard = progress.addInfo(subtaskDescription);
	
	const std::vector<Minterm> bits = listBits();
	Implicants primeImplicants;
	while (!implicants.empty())
	{
		if (progress.isVisible())
		{
			std::strcpy(subtaskDescription, std::to_string(implicants.size()).c_str());
			std::strcat(subtaskDescription, " left (");
			std::strcat(subtaskDescription, std::to_string(implicantSize--).c_str());
			std::strcat(subtaskDescription, " literals each)");
		}
		
		std::vector<Implicant> newImplicants;
		for (const Minterm &bit : bits)
		{
			step.substep();
			const Minterm mask = ~bit;
			static_assert(::maxBits == 32);
			const auto makeComparisonValue = [mask](const Implicant &implicant){ return ((static_cast<std::uint_fast64_t>(implicant.getMask()) << 32) | static_cast<std::uint_fast64_t>(implicant.getBits())) & static_cast<std::uint_fast64_t>(mask); };
			std::sort(implicants.begin(), implicants.end(), [makeComparisonValue](const std::pair<Implicant, bool> &x, const std::pair<Implicant, bool> &y){
					return makeComparisonValue(x.first) < makeComparisonValue(y.first);
				});
			std::pair<Implicant, bool> *previous = &implicants.front();
			for (auto iter = std::next(implicants.begin()); iter != implicants.end(); ++iter)
			{
				const bool maskMakesThemEqual = makeComparisonValue(previous->first) == makeComparisonValue(iter->first);
				if (maskMakesThemEqual)
				{
					Implicant newImplicant = previous->first;
					newImplicant.applyMask(mask);
					newImplicants.push_back(std::move(newImplicant));
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
		
		std::sort(newImplicants.begin(), newImplicants.end());
		newImplicants.erase(std::unique(newImplicants.begin(), newImplicants.end()), newImplicants.end());
		implicants.reserve(newImplicants.size());
		for (const auto &newImplicant : newImplicants)
			implicants.emplace_back(newImplicant, false);
	}
	return primeImplicants;
}

Implicants QuineMcCluskey::findPrimeImplicantsWithoutHeuristic(const Minterms &allowedMinterms, const Minterms &targetMinterms, const std::string &functionName)
{
	const std::string progressName = "Merging implicants of \"" + functionName + '"';
	Progress progress(Progress::Stage::SOLVING, progressName.c_str(), 3, false);
	
	Implicants primeImplicants = createPrimeImplicantsWithoutHeuristic(allowedMinterms, progress);
	removeDontCareOnlyImplicants(targetMinterms, primeImplicants, progress);
	cleanupImplicants(primeImplicants, progress);
	return primeImplicants;
}

Implicants QuineMcCluskey::findPrimeImplicants(const Minterms &allowedMinterms, const Minterms &targetMinterms, const std::string &functionName)
{
	switch (options::primeImplicantsHeuristic.getValue())
	{
	case options::PrimeImplicantsHeuristic::BRUTE_FORCE:
		brute_force:
		return findPrimeImplicantsWithoutHeuristic(allowedMinterms, targetMinterms, functionName);
		
	case options::PrimeImplicantsHeuristic::AUTO:
		if (::bits < 20)
			goto brute_force;
		else
			goto greedy;
		
	case options::PrimeImplicantsHeuristic::GREEDY:
		greedy:
		return findPrimeImplicantsWithHeuristic(allowedMinterms, targetMinterms, functionName);
	}
	// unreachable
	[[unlikely]]
	return {};
}

#ifndef NDEBUG
void QuineMcCluskey::validate(const Minterms &allowedMinterms, const Minterms &targetMinterms, const Implicants &implicants)
{
	{
		Implicants sortedImplicants = implicants;
		sortedImplicants.humanSort();
		assert(implicants == sortedImplicants);
	}
	assert(std::adjacent_find(implicants.cbegin(), implicants.cend()) == implicants.cend());
	Minterms missedTargetMinterms = targetMinterms;
	for (const Implicant &implicant : implicants)
	{
		assert(implicant.isAnyInMinterms(targetMinterms));
		assert(implicant.areAllInMinterms(allowedMinterms));
		implicant.removeFromMinterms(missedTargetMinterms);
	}
	assert(missedTargetMinterms.isEmpty());
}
#endif

QuineMcCluskey::solutions_t QuineMcCluskey::runPetricksMethod(Implicants &&primeImplicants, const Minterms &targetMinterms, const std::string &functionName)
{
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

QuineMcCluskey::solutions_t QuineMcCluskey::solve(const Minterms &allowedMinterms, const Minterms &targetMinterms, const std::string &functionName) const
{
	Implicants primeImplicants = findPrimeImplicants(allowedMinterms, targetMinterms, functionName);
#ifndef NDEBUG
	validate(allowedMinterms, targetMinterms, primeImplicants);
#endif
	return runPetricksMethod(std::move(primeImplicants), targetMinterms, functionName);
}
