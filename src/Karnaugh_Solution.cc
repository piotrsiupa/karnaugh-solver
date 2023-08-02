#include "./Karnaugh_Solution.hh"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>

#include "math.hh"


template<bits_t BITS>
Karnaugh_Solution<BITS>::Karnaugh_Solution(const minterms_t &minterms, const table_t &target, const table_t &dontCares) :
	minterms(minterms),
	target(target),
	dontCares(dontCares),
	solutionSpaceSize(std::pow(2.0, minterms.size()))
{
}

template<bits_t BITS>
bool Karnaugh_Solution<BITS>::adjustProgressInterval(const elapsedTime_t elapsedTime, progressCounter_t &progressInterval)
{
	static constexpr double targetInterval = 5.0;
	if (elapsedTime.count() < targetInterval)
	{
		const progressCounter_t intervalAdjustment = progressInterval * ((targetInterval - elapsedTime.count()) / elapsedTime.count());
		if (intervalAdjustment != 0)
		{
			progressInterval += intervalAdjustment;
			return false;
		}
	}
	static constexpr double magicConstantOfBetterTiming = 1.07;
	progressInterval *= magicConstantOfBetterTiming;
	return true;
}

template<bits_t BITS>
double Karnaugh_Solution<BITS>::estimateRemainingSolutionsFactor(const solution_t &currentSolution)
{
	double remaingSolutionsFactor = 1.0;
	double currentCutoffFactor = 1.0;
	std::size_t expectedValue = 0;
	for (const std::size_t x : currentSolution)
	{
		currentCutoffFactor /= 2;
		while (x != expectedValue)
		{
			remaingSolutionsFactor -= currentCutoffFactor;
			currentCutoffFactor /= 2;
			expectedValue += 1;
		}
		expectedValue += 1;
	}
	return remaingSolutionsFactor;
}

template<bits_t BITS>
void Karnaugh_Solution<BITS>::displayProgress(const solution_t &currentSolution, const timePoint_t &currentTime, const elapsedTime_t elapsedTime) const
{
	const std::chrono::duration<double> elapsedTimeSinceBest = currentTime - bestTime;
	const double remainingSolutionsFactor = estimateRemainingSolutionsFactor(currentSolution);
	const double progressGuess = (1.0 - remainingSolutionsFactor) * 100.0;
	const double remainingSolutions = calcBinomialCoefficientSum(minterms.size(), best.size()) * remainingSolutionsFactor;
	const double progressPercent = remainingSolutions / solutionSpaceSize * 100.0;
	
	std::clog << "Oops... This is taking a long time. (currently " << std::fixed << std::setprecision(1) << elapsedTime.count() << " seconds)\n";
	std::clog << "(Currently tested combination:";
	for (const std::size_t x : currentSolution)
		std::clog << ' ' << x;
	std::clog << ")\n";
	std::clog << "Estimation of remaining solution space: ~" << std::fixed << std::setprecision(3) << progressPercent << "% (~" << std::setprecision(0) << remainingSolutions << " combinations)\n";
	std::clog << "A very rough guess of the actual progress is ~" << std::fixed << std::setprecision(3) << progressGuess << "%.\n";
	if (isSolutionValid())
	{
		std::clog << "The best solution found so far uses " << best.size() << " out of " << minterms.size() << " minterms. (found " << elapsedTimeSinceBest.count() << " seconds ago)\n";
		std::clog << "Press Ctrl-C to abort further search and use this solution..." << std::flush;
	}
	else
	{
		std::clog << "No satisfactory solution was found yet.\n";
		std::clog << "Press Ctrl-C to abort further search and give up..." << std::flush;
	}
}

template<bits_t BITS>
bool Karnaugh_Solution<BITS>::processProgress(const solution_t &currentSolution, progressCounter_t &progressInterval, bool &progressIntervalAdjusted) const
{
	const timePoint_t currentTime = std::chrono::steady_clock::now();
	const std::chrono::duration<double> elapsedTime = currentTime - startTime;
	if (!progressIntervalAdjusted)
	{
		progressIntervalAdjusted = adjustProgressInterval(elapsedTime, progressInterval);
		if (!progressIntervalAdjusted)
			return false;
	}
	displayProgress(currentSolution, currentTime, elapsedTime);
	return true;
}

template<bits_t BITS>
void Karnaugh_Solution<BITS>::clearProgress()
{
	std::clog << "\033[2K\033[A\033[2K\033[A\033[2K\033[A\033[2K\033[A\033[2K\033[A\033[2K\r";
}

template<bits_t BITS>
typename Karnaugh_Solution<BITS>::table_t Karnaugh_Solution<BITS>::createMintermTable(const minterm_t minterm)
{
	table_t table;
	const number_t positiveMask = minterm.first;
	const number_t negativeMask = minterm.second ^ ((1 << BITS) - 1);
	for (number_t i = 0; i != 1 << BITS; ++i)
		table[(i | positiveMask) & negativeMask] = true;
	return table;
}

template<bits_t BITS>
void Karnaugh_Solution<BITS>::createMintermTables()
{
	const table_t cares = ~dontCares;
	mintermTables.resize(minterms.size());
	for (size_t i = 0; i != minterms.size(); ++i)
		mintermTables[i] = createMintermTable(minterms[i]) & cares;
}

template<bits_t BITS>
typename Karnaugh_Solution<BITS>::minterms_t Karnaugh_Solution<BITS>::removeEssentials()
{
	createMintermTables();
	
	table_t all, repeated;
	for (const table_t &x : mintermTables)
	{
		repeated |= all & x;
		all |= x;
	}
	const table_t unique = all & ~repeated;
	
	minterms_t essentials;
	table_t essentialTable;
	for (std::size_t i = 0; i != minterms.size(); ++i)
	{
		if ((mintermTables[i] & unique).any())
		{
			essentials.push_back(minterms[i]);
			essentialTable |= mintermTables[i];
			mintermTables[i].reset();
		}
	}
	
	const table_t nonEssentialsMask = ~essentialTable;
	for (table_t &mintermTable : mintermTables)
		mintermTable &= nonEssentialsMask;
	target &= nonEssentialsMask;
	
	for (std::size_t i = 0; i != minterms.size(); ++i)
		if (mintermTables[i].none())
			minterms[i] = {0, 0};
	
	minterms.erase(std::remove_if(minterms.begin(), minterms.end(), [](const minterm_t x){ return x.first == 0 && x.second == 0; }), minterms.end());
	mintermTables.erase(std::remove_if(mintermTables.begin(), mintermTables.end(), [](const table_t &x){ return x.none(); }), mintermTables.end());
	
	return essentials;
}

template<bits_t BITS>
void Karnaugh_Solution<BITS>::removeUnnededMinterms(solution_t &current, mintermTables_t &currentTables)
{
	table_t remainingTable;
	for (std::size_t i = current.size() - 1; i != 0; --i)
	{
		if (((currentTables[i] | remainingTable) & target) == target)
			current.erase(std::next(current.begin(), i));
		else
			remainingTable |= mintermTables[current[i]];
	}
	if (best.size() != current.size())
	{
		best = current;
		bestTime = std::chrono::steady_clock::now();
		currentTables.resize(current.size() + 1);
		for (std::size_t i = 0; i != current.size(); ++i)
			currentTables[i + 1] = currentTables[i] | mintermTables[current[i]];
	}
}

template<bits_t BITS>
void Karnaugh_Solution<BITS>::solve()
{
	solution = removeEssentials();
	
	if (!minterms.empty())
	{
		best.resize(minterms.size() + 1);
		
		solution_t current;
		current.reserve(minterms.size());
		current.emplace_back(0);
		mintermTables_t currentTables;
		currentTables.reserve(minterms.size() + 1);
		currentTables.emplace_back();
		
		bestTime = startTime = std::chrono::steady_clock::now();
		std::uintmax_t progressInterval = 65536;
		bool progressIntervalAdjusted = false;
		
		for (std::uintmax_t progressCounter = 0;; ++progressCounter)
		{
			if (progressCounter == progressInterval)
			{
				if (progressIntervalAdjusted)
					clearProgress();
				if (intSignalFlag)
					break;
				if (processProgress(current, progressInterval, progressIntervalAdjusted))
					progressCounter = 0;
			}
			
			currentTables.push_back(currentTables.back() | mintermTables[current.back()]);
			if (currentTables.back() == target)
			{
				best = current;
				bestTime = std::chrono::steady_clock::now();
				if (current.size() == 1)
					break;
				removeUnnededMinterms(current, currentTables);
				currentTables.pop_back();
				goto go_back;
			}
			else if (currentTables.back() == currentTables[currentTables.size() - 2])
			{
				currentTables.pop_back();
				goto go_next;
			}
			if (current.size() + 1 == best.size())
			{
				currentTables.pop_back();
				goto go_next;
			}
			current.emplace_back(current.back() + 1);
			if (current.back() != minterms.size())
				continue;
			go_back:
			current.pop_back();
			currentTables.pop_back();
			if (current.empty())
			{
				if (progressIntervalAdjusted)
					clearProgress();
				break;
			}
			go_next:
			if (++current.back() == minterms.size())
				goto go_back;
		}
		
		std::clog << std::flush;
		
		solution.reserve(solution.size() + best.size());
		for (const auto &x : best)
			solution.push_back(minterms[x]);
	}
}

template<bits_t BITS>
void Karnaugh_Solution<BITS>::prettyPrintSolution() const
{
	table_t solutionTable;
	for (const auto &x : solution)
		solutionTable |= createMintermTable(x);
	std::cout << "best fit:\n";
	Karnaugh<BITS>::prettyPrintTable(solutionTable);
}

template<bits_t BITS>
Karnaugh_Solution<BITS> Karnaugh_Solution<BITS>::solve(const minterms_t &allMinters, const table_t &target, const table_t &dontCares)
{
	intSignalFlag = false;
	
	Karnaugh_Solution karnaugh_solver(allMinters, target, dontCares);
	karnaugh_solver.solve();
	return karnaugh_solver;
}


template class Karnaugh_Solution<2>;
template class Karnaugh_Solution<3>;
template class Karnaugh_Solution<4>;
template class Karnaugh_Solution<5>;
template class Karnaugh_Solution<6>;
template class Karnaugh_Solution<7>;
template class Karnaugh_Solution<8>;
template class Karnaugh_Solution<9>;
template class Karnaugh_Solution<10>;
template class Karnaugh_Solution<11>;
template class Karnaugh_Solution<12>;
template class Karnaugh_Solution<13>;
template class Karnaugh_Solution<14>;
template class Karnaugh_Solution<15>;
template class Karnaugh_Solution<16>;
