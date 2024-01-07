#include "./QuineMcCluskey.hh"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <functional>
#include <iostream>
#include <iterator>
#include <numeric>
#include <string>

#include "options.hh"
#include "PetricksMethod.hh"
#include "Progress.hh"
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

Implicants QuineMcCluskey::createInitialImplicants(const Minterms &minterms, Progress &progress)
{
	const auto infoGuard = progress.addInfo("creating initial list of implicants");
	progress.step();
	progress.substep([](){ return -0.0; }, true);
	auto progressStep = progress.makeCountingStepHelper(minterms.getSize());
	
	Implicants implicants;
	implicants.reserve(minterms.getSize());
	for (const Minterm minterm : minterms)
	{
		progressStep.substep();
		implicants.push_back(Implicant(minterm));
	}
	return implicants;
}
	
void QuineMcCluskey::mergeImplicants(Implicants &implicants, Progress *const progress)
{
	if (progress != nullptr)
	{
		const auto infoGuard = progress->addInfo("merging implicants with heuristic");
		progress->step();
		progress->substep([](){ return -0.0; }, true);
	}
	
	const std::vector<Minterm> bits = listBits();
	
	::bits_t i = 0;
	const Progress::calcStepCompletion_t calcStepCompletion = [&i = std::as_const(i)]() { return -static_cast<Progress::completion_t>(i) / static_cast<Progress::completion_t>(::bits); };
	for (i = 0; i != ::bits; ++i)
	{
		std::vector<Implicant>::iterator currentEnd = implicants.end();
		if (progress != nullptr)
			progress->substep(calcStepCompletion, true);
		const Minterm mask = ~(Minterm(1) << i);
		std::sort(implicants.begin(), currentEnd, [mask](const Implicant &x, const Implicant &y){
				const Minterm xm = x.getRawMask(), ym = y.getRawMask();
				if (xm != ym)
					return xm < ym;
				const Minterm xmb = x.getRawBits() & mask, ymb = y.getRawBits() & mask;
				return xmb < ymb;
			});
		for (std::vector<Implicant>::iterator current = implicants.begin(), next = std::next(implicants.begin()); next != currentEnd; current = next, ++next)
		{
			if (current->getRawMask() == next->getRawMask() && (current->getRawBits() & mask) == (next->getRawBits() & mask))
			{
				current->applyMask(mask);
				*next = Implicant::none();
				if (++next == currentEnd)
					break;
			}
		}
		implicants.erase(std::remove(implicants.begin(), currentEnd, Implicant::none()), implicants.end());
	}
}

void QuineMcCluskey::mergeAndExtendImplicants(Implicants &implicants, Progress &progress)
{
	const auto infoGuard = progress.addInfo("expanding and merging implicants");
	progress.step();
	std::size_t extendedCount = implicants.size();
	const Progress::calcStepCompletion_t calcStepCompletion = [&implicants = std::as_const(implicants), &extendedCount = std::as_const(extendedCount)]() { return -1.0 + std::log(static_cast<Progress::completion_t>(std::max(std::size_t(1), extendedCount))) / std::log(static_cast<Progress::completion_t>(implicants.size())); };
	while (extendedCount != 0)
	{
		progress.substep(calcStepCompletion, true);
		extendedCount = 0;
		
		mergeImplicants(implicants, nullptr);
		
		std::sort(implicants.begin(), implicants.end(), [](const Implicant &x, const Implicant &y){ return x.getBitCount() > y.getBitCount(); });
		
		for (auto iter = implicants.begin(), firstBiggerIter = implicants.begin();; ++iter)
		{
			if (iter == firstBiggerIter)
				if ((firstBiggerIter = std::partition_point(iter, implicants.end(), [currentCount = iter->getBitCount()](const Implicant &x){ return x.getBitCount() == currentCount; })) == implicants.end())
					break;
			Implicant bestResult = Implicant::none();
			for (auto jiter = firstBiggerIter; jiter != implicants.end(); ++jiter)
			{
				Implicant result = Implicant::findBiggestInUnion(*iter, *jiter);
				if (result != Implicant::none() && (result.getRawMask() & iter->getRawMask()) == result.getRawMask())
					if (bestResult == Implicant::none() || result.getRawMask() > bestResult.getRawMask())
						bestResult = std::move(result);
			}
			if (bestResult != Implicant::none())
			{
				*iter = std::move(bestResult);
				++extendedCount;
			}
		}
	}
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

Implicants QuineMcCluskey::findPrimeImplicants(Minterms allowedMinterms, const std::string &functionName)
{
	if (allowedMinterms.getSize() == 0)
		return {Implicant::none()};
	else if (allowedMinterms.getSize() - 1 == ::maxMinterm)
		return {Implicant::all()};
	
	const std::string progressName = "Finding prime impl. of \"" + functionName + '"';
	Progress progress(Progress::Stage::SOLVING, progressName.c_str(), 4, false);
	
	Implicants implicants = createInitialImplicants(allowedMinterms, progress);
	mergeAndExtendImplicants(implicants, progress);
	createAlternativeImplicants(implicants, progress);
	cleanupImplicants(implicants, progress);
	return implicants;
}

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
	Implicants primeImplicants = findPrimeImplicants(allowedMinterms, functionName);
	return runPetricksMethod(std::move(primeImplicants), targetMinterms, functionName);
}
