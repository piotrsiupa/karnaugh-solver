#include "./QuineMcCluskey.hh"

#include <algorithm>
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


std::vector<Minterm>::iterator QuineMcCluskey::mergeSomeMinterms(const std::vector<Minterm>::iterator begin, const std::vector<Minterm>::iterator end, std::vector<Minterm>::iterator remaining, std::vector<Minterm> bits, std::vector<Implicant> &implicants, Progress::CountingStepHelper<std::size_t> &progressStep)
{
	const std::uint_fast32_t mintermCount = static_cast<std::uint_fast32_t>(std::distance(begin, end));
	
	switch (mintermCount)
	{
	case 0:
#if defined(__GNUC__)
		__builtin_unreachable();
#elif defined(_MSC_VER)
		__assume(false);
#endif
	[[likely]] case 1:
		*remaining = *begin;
		return std::next(remaining);
		
	default:
		std::vector<std::uint_fast32_t> ratings(bits.size());
		for (std::uint_fast8_t i = 0; i != bits.size(); ++i)
		{
			const Minterm bit = bits[i];
			std::uint_fast32_t &rating = ratings[i];
			for (std::vector<Minterm>::const_iterator jiter = begin; jiter != end; ++jiter)
				if ((*jiter & bit) != 0)
					++rating;
		}
		
		for (std::uint_fast32_t &rating : ratings)
			rating = std::min(rating, mintermCount - rating) - 1;
		
		const std::uint_fast32_t maxCount = std::uint_fast32_t(0) - 1;
		{
			std::size_t i, j;
			for (i = 0, j = 0; i != ratings.size(); ++i)
			{
				if (ratings[i] != maxCount)
				{
					ratings[j] = ratings[i];
					bits[j] = bits[i];
					++j;
				}
			}
			ratings.resize(j);
			bits.resize(j);
		}
		if (mintermCount == std::uint_fast64_t(1) << bits.size())
		{
			const Minterm mask = ::maxMinterm & ~std::accumulate(bits.cbegin(), bits.cend(), 0, std::bit_or<Minterm>());
			implicants.emplace_back(*begin & mask, mask);
			progressStep.substep(mintermCount, false);
			return remaining;
		}
		
		const std::size_t chosenIndex = std::distance(ratings.cbegin(), std::min_element(ratings.cbegin(), ratings.cend()));
		const Minterm chosenBit = bits[chosenIndex];
		bits.erase(std::next(bits.begin(), chosenIndex));
		
		const std::vector<Minterm>::iterator middle = std::partition(begin, end, [chosenBit](const Minterm &minterm){ return (minterm & chosenBit) == 0; });
		
		remaining = mergeSomeMinterms(begin, middle, remaining, bits, implicants, progressStep);
		remaining = mergeSomeMinterms(middle, end, remaining, std::move(bits), implicants, progressStep);
		return remaining;
	}
}


Implicants QuineMcCluskey::findPrimeImplicants(Minterms allowedMinterms, const std::string &functionName) const
{
	if (allowedMinterms.getSize() == 0)
		return {Implicant::none()};
	else if (allowedMinterms.getSize() - 1 == ::maxMinterm)
		return {Implicant::all()};
	
	const std::string progressName = "Finding prime impl. of \"" + functionName + '"';
	Progress progress(Progress::Stage::SOLVING, progressName.c_str(), 3, false);
	
	std::vector<Implicant> newImplicants;
	
	{
		const auto infoGuard = progress.addInfo("finding prime implicants with heuristic");
		progress.step();
		progress.substep([](){ return -0.0; }, true);
		auto progressStep = progress.makeCountingStepHelper(allowedMinterms.getSize());
		std::vector<Minterm> bits;
		if (::bits > 0)
		{
			const Minterm maxBit = Minterm(1) << (::bits - 1);
			bits.reserve(::bits);
			for (Minterm bit = 1;; bit <<= 1)
			{
				bits.push_back(bit);
				if (bit == maxBit)
					break;
			}
		}
		std::vector<Minterm> minterms;
		minterms.reserve(allowedMinterms.getSize());
		for (const Minterm minterm : allowedMinterms)
			minterms.push_back(minterm);
		Progress::cerr() << "= " << minterms.size() << '\n';
		while (!minterms.empty())
		{
			const auto newEnd = mergeSomeMinterms(minterms.begin(), minterms.end(), minterms.begin(), bits, newImplicants, progressStep);
			if (newEnd == minterms.cend())
				break;
			minterms.erase(newEnd, minterms.end());
		}
		for (const Minterm &minterm : minterms)
			newImplicants.push_back(Implicant(minterm));
	}
	
	Implicants primeImplicants;
	{
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
	}
	
	{
		const auto infoGuard = progress.addInfo("sorting prime implicants");
		progress.step();
		progress.substep([](){ return -0.0; }, true);
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
