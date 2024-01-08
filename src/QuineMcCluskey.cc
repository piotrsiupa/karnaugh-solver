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

Implicants QuineMcCluskey::createImplicants(const Minterms &allowedMinterms, const Minterms &targetMinterms, Progress &progress)
{
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
		if (bits.size() >= 2)
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
		implicant.addToMinterms(touchedMinterms);
		implicants.push_back(std::move(implicant));
	}
	return implicants;
}

void QuineMcCluskey::createAlternativeImplicants(Implicants &newImplicants, Progress &progress)
{
	Implicants primeImplicants;
	progress.step();
	std::vector<Implicant> implicants;
	std::vector<bool> usedImplicants, obsoleteImplicants;
	char infoText[0x80] = "";
	const auto infoGuard = progress.addInfo(infoText);
	::bits_t bitCountLimit;
	std::size_t i;
	Progress::calcStepCompletion_t calcStepCompletion = [&bitCountLimit = std::as_const(bitCountLimit), &implicants = std::as_const(implicants), &i = std::as_const(i)](){
			const Progress::completion_t majorProgress = static_cast<Progress::completion_t>(::bits - bitCountLimit) / static_cast<Progress::completion_t>(::bits + 1);
			const Progress::completion_t minorProgress = static_cast<Progress::completion_t>(i) / static_cast<Progress::completion_t>(implicants.size()) / static_cast<Progress::completion_t>(::bits + 1);
			return majorProgress + minorProgress;
		};
	for (bitCountLimit = ::bits; !newImplicants.empty() || !implicants.empty(); --bitCountLimit)
	{
		i = 0;
		progress.substep(calcStepCompletion, true);
		
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
			std::strcpy(infoText, "merging impl. stage ");
			std::strcat(infoText, std::to_string(::bits - bitCountLimit + 1).c_str());
			std::strcat(infoText, "/");
			std::strcat(infoText, std::to_string(::bits + 1).c_str());
			std::strcat(infoText, " (");
			std::strcat(infoText, std::to_string(firstNewImplicant - implicants.cbegin()).c_str());
			std::strcat(infoText, "+");
			std::strcat(infoText, std::to_string(implicants.cend() - firstNewImplicant).c_str());
			std::strcat(infoText, ")");
		}
		
		for (i = 0; i != implicants.size(); ++i)
		{
			progress.substep(calcStepCompletion);
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
	newImplicants = std::move(primeImplicants);
}
	
void QuineMcCluskey::cleanupImplicants(Implicants &implicants, Progress &progress)
{
	const auto infoGuard = progress.addInfo("sorting prime implicants");
	progress.step();
	progress.substep([](){ return -0.0; }, true);
	
	implicants.humanSort();
	implicants.shrink_to_fit();
}

Implicants QuineMcCluskey::findPrimeImplicants(const Minterms &allowedMinterms, const Minterms &targetMinterms, const std::string &functionName)
{
	if (allowedMinterms.getSize() == 0)
		return {Implicant::none()};
	else if (allowedMinterms.getSize() - 1 == ::maxMinterm)
		return {Implicant::all()};
	
	const std::string progressName = "Finding prime impl. of \"" + functionName + '"';
	Progress progress(Progress::Stage::SOLVING, progressName.c_str(), 3, false);
	
	Implicants implicants = createImplicants(allowedMinterms, targetMinterms, progress);
	createAlternativeImplicants(implicants, progress);
	cleanupImplicants(implicants, progress);
	return implicants;
}

#ifndef NDEBUG
void QuineMcCluskey::validate(const Minterms &allowedMinterms, Minterms targetMinterms, const Implicants &implicants)
{
	for (const Implicant &implicant : implicants)
	{
		assert(implicant.areAllInMinterms(allowedMinterms));
		implicant.removeFromMinterms(targetMinterms);
	}
	assert(targetMinterms.isEmpty());
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
	Implicants primeImplicants = findPrimeImplicants(allowedMinterms, allowedMinterms, functionName);
#ifndef NDEBUG
	validate(allowedMinterms, targetMinterms, primeImplicants);
#endif
	return runPetricksMethod(std::move(primeImplicants), targetMinterms, functionName);
}
