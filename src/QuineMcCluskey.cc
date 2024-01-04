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
		
	case 2:
	{
		const Minterm minterm0 = *begin, minterm1 = *std::next(begin, 1);
		const Minterm difference = minterm0 ^ minterm1;
		const bool differsBy1Bit = (difference & (difference - 1)) == 0;
		if (differsBy1Bit)
		{
			implicants.emplace_back(minterm0 & ~difference, ::maxMinterm & ~difference);
			progressStep.substep(2, false);
		}
		else
		{
			*(remaining++) = minterm0;
			*(remaining++) = minterm1;
		}
		return remaining;
	}
	
	case 3:
	{
		const Minterm minterm0 = *begin, minterm1 = *std::next(begin, 1), minterm2 = *std::next(begin, 2);
		Minterm difference = minterm0 ^ minterm1;
		bool differsBy1Bit = (difference & (difference - 1)) == 0;
		if (differsBy1Bit)
		{
			implicants.emplace_back(minterm0 & ~difference, ::maxMinterm & ~difference);
			*(remaining++) = minterm2;
			progressStep.substep(2, false);
		}
		else
		{
			difference = minterm0 ^ minterm2;
			differsBy1Bit = (difference & (difference - 1)) == 0;
			if (differsBy1Bit)
			{
				implicants.emplace_back(minterm0 & ~difference, ::maxMinterm & ~difference);
				*(remaining++) = minterm1;
				progressStep.substep(2, false);
			}
			else
			{
				difference = minterm1 ^ minterm2;
				differsBy1Bit = (difference & (difference - 1)) == 0;
				if (differsBy1Bit)
				{
					implicants.emplace_back(minterm1 & ~difference, ::maxMinterm & ~difference);
					*(remaining++) = minterm0;
					progressStep.substep(2, false);
				}
				else
				{
					*(remaining++) = minterm0;
					*(remaining++) = minterm1;
					*(remaining++) = minterm2;
				}
			}
		}
		return remaining;
	}
	
	case 4:
	{
		Minterm minterm0 = *begin, minterm1 = *std::next(begin, 1), minterm2 = *std::next(begin, 2), minterm3 = *std::next(begin, 3);
		if (minterm0 > minterm2)
			std::swap(minterm0, minterm2);
		if (minterm1 > minterm3)
			std::swap(minterm1, minterm3);
		Minterm difference = minterm0 ^ minterm1;
		bool differsBy1Bit = (difference & (difference - 1)) == 0;
		if (differsBy1Bit)
		{
			const Minterm difference1 = minterm2 ^ minterm3;
			differsBy1Bit = (difference1 & (difference1 - 1)) == 0;
			if (differsBy1Bit)
			{
				const Minterm difference2 = (minterm0 & minterm1) ^ (minterm2 & minterm3);
				differsBy1Bit = (difference2 & (difference2 - 1)) == 0;
				if (differsBy1Bit && difference == difference1)
				{
					implicants.emplace_back(minterm0 & ~(difference | difference2), ::maxMinterm & ~(difference | difference2));
				}
				else
				{
					implicants.emplace_back(minterm0 & ~difference, ::maxMinterm & ~difference);
					implicants.emplace_back(minterm2 & ~difference1, ::maxMinterm & ~difference1);
				}
				progressStep.substep(4, false);
			}
			else
			{
				implicants.emplace_back(minterm0 & ~difference, ::maxMinterm & ~difference);
				*(remaining++) = minterm2;
				*(remaining++) = minterm3;
				progressStep.substep(2, false);
			}
		}
		else
		{
			difference = minterm1 ^ minterm2;
			differsBy1Bit = (difference & (difference - 1)) == 0;
			if (differsBy1Bit)
			{
				implicants.emplace_back(minterm1 & ~difference, ::maxMinterm & ~difference);
				progressStep.substep(2, false);
			}
			else
			{
				*(remaining++) = minterm1;
				*(remaining++) = minterm2;
			}
			difference = minterm0 ^ minterm3;
			differsBy1Bit = (difference & (difference - 1)) == 0;
			if (differsBy1Bit)
			{
				implicants.emplace_back(minterm0 & ~difference, ::maxMinterm & ~difference);
				progressStep.substep(2, false);
			}
			else
			{
				*(remaining++) = minterm0;
				*(remaining++) = minterm3;
			}
		}
		return remaining;
	}
	
	default:
		std::vector<std::uint_fast32_t> ratings(bits.size());
		for (std::uint_fast8_t i = 0; i != bits.size(); ++i)
		{
			const Minterm bit = bits[i];
			std::uint_fast32_t &rating = ratings[i];
			for (std::vector<Minterm>::const_iterator jiter = begin; jiter != end; ++jiter)
				if ((*jiter & bit) != 0)
					++rating;
			rating = std::min(rating, mintermCount - rating);
		}
		
		{
			std::size_t i, j;
			for (i = 0, j = 0; i != ratings.size(); ++i)
			{
				if (ratings[i] != 0)
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
	
	std::vector<Implicant> newImplicants;
	
	{
		const auto infoGuard = progress.addInfo("finding prime implicants with heuristic");
		progress.step();
		progress.substep([](){ return -0.0; }, true);
		auto progressStep = progress.makeCountingStepHelper(allowedMinterms.getSize());
		std::vector<Minterm> minterms;
		minterms.reserve(allowedMinterms.getSize());
		for (const Minterm minterm : allowedMinterms)
			minterms.push_back(minterm);
		Progress::cerr() << "= " << minterms.size() << '\n';
		newImplicants.reserve(minterms.size() / 4 + minterms.size() / 8);
		while (!minterms.empty())
		{
			const auto newEnd = mergeSomeMinterms(minterms.begin(), minterms.end(), minterms.begin(), bits, newImplicants, progressStep);
			const std::size_t erasedCount = std::distance(newEnd, minterms.end());
			minterms.erase(newEnd, minterms.end());
			if (erasedCount < minterms.size())
				break;
		}
		newImplicants.reserve(newImplicants.size() + minterms.size());
		for (const Minterm &minterm : minterms)
			newImplicants.push_back(Implicant(minterm));
	}
	
	for (::bits_t bitCount = ::bits; bitCount != 0; --bitCount)
	{
		const std::vector<Implicant>::iterator partitionEnd = std::partition(newImplicants.begin(), newImplicants.end(), [bitCount](const Implicant &implicant){ return implicant.getBitCount() == bitCount; });
		if (partitionEnd == newImplicants.begin())
			continue;
		std::vector<Implicant>::iterator currentEnd = partitionEnd;
		for (const Minterm bit : bits)
		{
			const Minterm mask = ~bit;
			std::sort(newImplicants.begin(), currentEnd, [mask](const Implicant &x, const Implicant &y){
					const Minterm xm = x.getRawMask(), ym = y.getRawMask();
					if (xm != ym)
						return xm < ym;
					const Minterm xmb = x.getRawBits() & mask, ymb = y.getRawBits() & mask;
					return xmb < ymb;
				});
			for (std::vector<Implicant>::iterator current = newImplicants.begin(), next = std::next(newImplicants.begin()); next != currentEnd; current = next, ++next)
			{
				if (current->getRawMask() == next->getRawMask() && (current->getRawBits() & mask) == (next->getRawBits() & mask))
				{
					current->applyMask(mask);
					*next = Implicant::none();
					if (++next == currentEnd)
						break;
				}
			}
			currentEnd = std::remove(newImplicants.begin(), currentEnd, Implicant::none());
		}
		newImplicants.erase(currentEnd, partitionEnd);
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
