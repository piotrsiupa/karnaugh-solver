#include "./QuineMcCluskey.hh"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <map>

#include "options.hh"
#include "PetricksMethod.hh"


std::vector<Minterm> QuineMcCluskey::bitMasks;

void QuineMcCluskey::makeBitMasks()
{
	if (::bits == 0)
		return;
	
	const Minterm maxBitMask = Minterm(1) << (::bits - 1);
	bitMasks.reserve(::bits);
	for (Minterm bitMask = 1;; bitMask <<= 1)
	{
		bitMasks.push_back(bitMask);
		if (bitMask == maxBitMask)
			break;
	}
}

void QuineMcCluskey::removeDontCareOnlyImplicants(Implicants &implicants, Progress &progress) const
{
	const auto infoGuard = progress.addInfo("removing useless prime implicants");
	progress.step();
	progress.substep([](){ return -0.0; }, true);
	
	const auto [eraseBegin, eraseEnd] = std::ranges::remove_if(implicants.begin(), implicants.end(), [&targetMinterms = *targetMinterms](const Implicant &implicant){ return !implicant.isAnyInMinterms(targetMinterms); });
	implicants.erase(eraseBegin, eraseEnd);
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

void QuineMcCluskey::refineHeuristicImplicant(const Minterm initialMinterm, Implicant &implicant) const
{
	for (auto iter = bitMasks.cbegin(); iter != std::ranges::prev(bitMasks.cend()); ++iter)
	{
		const Minterm removedBit = *iter;
		if ((removedBit & implicant.getMask()) != 0)
			continue;
		Implicant implicantVariant(implicant.getBits() | (initialMinterm & removedBit), implicant.getMask() | removedBit);
		for (auto jiter = std::ranges::next(iter); jiter != bitMasks.cend(); ++jiter)
		{
			const Minterm bit = *jiter;
			if ((bit & implicantVariant.getMask()) == 0)
				continue;
			if (Implicant(implicantVariant.getBits() ^ bit, implicantVariant.getMask()).areAllInMinterms(*allowedMinterms))
				implicantVariant.applyMask(~bit);
		}
		if (implicantVariant.getBitCount() <= implicant.getBitCount())
			implicant = std::move(implicantVariant);
	}
}

Implicants QuineMcCluskey::createImplicantsWithHeuristic(Progress &progress) const
{
	const std::uint_fast8_t adjustmentPasses = getAdjustmentPasses();
	
	const auto infoGuard = progress.addInfo("Creating implicants with a heuristic");
	progress.step();
	auto progressStep = progress.makeCountingStepHelper<Minterm>(::maxMinterm + 1);
	
	Minterms neededMinterms = *targetMinterms;
	Implicants implicants;
	Minterm previousInitialMinterm = Minterm(0) - 1;
	for (const Minterm initialMinterm : neededMinterms)
	{
		progressStep.substep(initialMinterm - previousInitialMinterm);
		previousInitialMinterm = initialMinterm;
		Implicant implicant(initialMinterm);
		for (const Minterm bitMask : bitMasks)
		{
			if (Implicant(implicant.getBits() ^ bitMask, implicant.getMask()).areAllInMinterms(*allowedMinterms))
				implicant.applyMask(~bitMask);
		}
		for (std::uint_fast64_t i = 0; i != adjustmentPasses; ++i)
			refineHeuristicImplicant(initialMinterm, implicant);
		implicant.removeFromMinterms(neededMinterms);
		implicants.push_back(std::move(implicant));
	}
	return implicants;
}

Implicants QuineMcCluskey::findPrimeImplicantsWithHeuristic()
{
	if (allowedMinterms->empty())
		return {Implicant::none()};
	else if (allowedMinterms->full())
		return {Implicant::all()};
	
	const std::string progressName = "Finding prime impl. of \"" + functionName + '"';
	Progress progress(Progress::Stage::SOLVING, progressName.c_str(), 2, false);
	
	Implicants implicants = createImplicantsWithHeuristic(progress);
	allowedMinterms.reset();
	cleanupImplicants(implicants, progress);
	return implicants;
}

Implicants QuineMcCluskey::createPrimeImplicantsWithoutHeuristic(Progress &progress)
{
	Progress::CountingStepHelper step = progress.makeCountingStepHelper(static_cast<std::size_t>(::bits + 1) * static_cast<std::size_t>(::bits));
	progress.step();
	
	std::vector<std::pair<Implicant, bool>> implicants;
	{
		const auto infoGuard = progress.addInfo("preparing initial list of implicants");
		progress.substep([](){ return -0.0; }, true);
		implicants.reserve(allowedMinterms->size());
		for (const Minterm &minterm : *allowedMinterms)
			implicants.emplace_back(Implicant{minterm}, false);
		allowedMinterms.reset();
	}
	
	::bits_t implicantSize = ::bits;
	char subtaskDescription[96] = "";
	const auto infoGuard = progress.addInfo(subtaskDescription);
	
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
		for (const Minterm &bitMask : bitMasks)
		{
			step.substep();
			static_assert(::maxBits == 32);
			const std::uint_fast64_t mask = ~static_cast<std::uint64_t>(bitMask);
			const auto makeComparisonValue = [mask](const Implicant &implicant){ return ((static_cast<std::uint_fast64_t>(implicant.getMask()) << 32) | static_cast<std::uint_fast64_t>(implicant.getBits())) & static_cast<std::uint_fast64_t>(mask); };
			std::ranges::sort(implicants.begin(), implicants.end(), [makeComparisonValue](const std::pair<Implicant, bool> &x, const std::pair<Implicant, bool> &y){
					return makeComparisonValue(x.first) < makeComparisonValue(y.first);
				});
			std::pair<Implicant, bool> *previous = &implicants.front();
			for (auto iter = std::next(implicants.begin()); iter != implicants.end(); ++iter)
			{
				const bool maskMakesThemEqual = makeComparisonValue(previous->first) == makeComparisonValue(iter->first);
				if (maskMakesThemEqual)
				{
					Implicant newImplicant = previous->first;
					newImplicant.applyMask(static_cast<Minterm>(mask));
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
		
		std::ranges::sort(newImplicants.begin(), newImplicants.end());
		const auto [eraseBegin, eraseEnd] = std::ranges::unique(newImplicants.begin(), newImplicants.end());
		newImplicants.erase(eraseBegin, eraseEnd);
		implicants.reserve(newImplicants.size());
		for (const auto &newImplicant : newImplicants)
			implicants.emplace_back(newImplicant, false);
	}
	return primeImplicants;
}

Implicants QuineMcCluskey::findPrimeImplicantsWithoutHeuristic()
{
	const std::string progressName = "Merging implicants of \"" + functionName + '"';
	Progress progress(Progress::Stage::SOLVING, progressName.c_str(), 3, false);
	
	Implicants primeImplicants = createPrimeImplicantsWithoutHeuristic(progress);
	removeDontCareOnlyImplicants(primeImplicants, progress);
	cleanupImplicants(primeImplicants, progress);
	return primeImplicants;
}

Implicants QuineMcCluskey::findPrimeImplicants()
{
	switch (options::primeImplicantsHeuristic.getValue())
	{
	case options::PrimeImplicantsHeuristic::BRUTE_FORCE:
		brute_force:
		return findPrimeImplicantsWithoutHeuristic();
		
	case options::PrimeImplicantsHeuristic::AUTO:
		if (::bits < 20)
			goto brute_force;
		else
			goto greedy;
		
	case options::PrimeImplicantsHeuristic::GREEDY:
		greedy:
		return findPrimeImplicantsWithHeuristic();
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
	assert(std::ranges::adjacent_find(implicants.cbegin(), implicants.cend()) == implicants.cend());
	Minterms missedTargetMinterms = targetMinterms;
	for (const Implicant &implicant : implicants)
	{
		assert(implicant.isAnyInMinterms(targetMinterms));
		assert(implicant.areAllInMinterms(allowedMinterms));
		implicant.removeFromMinterms(missedTargetMinterms);
	}
	assert(missedTargetMinterms.empty());
	if (implicants.size() <= 250000)
		for (Implicants::const_iterator iter = implicants.cbegin(); iter != implicants.cend(); ++iter)
			for (Implicants::const_iterator jiter = std::ranges::next(iter); jiter != implicants.cend(); ++jiter)
				assert(!iter->contains(*jiter) && !jiter->contains(*iter));
}
#endif

QuineMcCluskey::solutions_t QuineMcCluskey::runPetricksMethod(Implicants &&primeImplicants)
{
	if (primeImplicants.size() <= PetricksMethod<std::uint8_t>::MAX_PRIME_IMPL_COUNT)
		return PetricksMethod<std::uint8_t>::solve(*targetMinterms, std::move(primeImplicants), functionName);
	else if (primeImplicants.size() <= PetricksMethod<std::uint16_t>::MAX_PRIME_IMPL_COUNT)
		return PetricksMethod<std::uint16_t>::solve(*targetMinterms, std::move(primeImplicants), functionName);
	else if (primeImplicants.size() <= PetricksMethod<std::uint32_t>::MAX_PRIME_IMPL_COUNT)
		return PetricksMethod<std::uint32_t>::solve(*targetMinterms, std::move(primeImplicants), functionName);
	else if (primeImplicants.size() <= PetricksMethod<std::uint64_t>::MAX_PRIME_IMPL_COUNT)
		return PetricksMethod<std::uint64_t>::solve(*targetMinterms, std::move(primeImplicants), functionName);
	else
		std::cerr << "The number of prime implicants is ridiculous and this has no right to work! I won't even try.\n";
	return {};
}

QuineMcCluskey::solutions_t QuineMcCluskey::solve() &&
{
#ifdef NDEBUG
	Implicants primeImplicants = findPrimeImplicants();
#else
	std::shared_ptr<const Minterms> allowedMintermsForValidation = allowedMinterms;
	Implicants primeImplicants = findPrimeImplicants();
	validate(*allowedMintermsForValidation, *targetMinterms, primeImplicants);
	allowedMintermsForValidation.reset();
#endif
	return runPetricksMethod(std::move(primeImplicants));
}
