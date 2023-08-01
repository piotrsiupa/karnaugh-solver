#include "./Karnaugh_Solution.hh"

#include <cmath>
#include <iomanip>
#include <iostream>

#include "math.hh"


template<bits_t BITS>
Karnaugh_Solution<BITS>::Karnaugh_Solution(const Karnaugh<BITS> &karnaugh) :
	karnaugh(karnaugh),
	solutionSpaceSize(std::pow(2.0, karnaugh.allMinterms.size()))
{
}

template<bits_t BITS>
void Karnaugh_Solution<BITS>::createMintermTables()
{
	mintermTables.resize(karnaugh.allMinterms.size());
	for (size_t i = 0; i != karnaugh.allMinterms.size(); ++i)
	{
		const number_t positiveMask = karnaugh.allMinterms[i].first;
		const number_t negativeMask = karnaugh.allMinterms[i].second ^ ((1 << BITS) - 1);
		for (number_t j = 0; j != 1 << BITS; ++j)
			mintermTables[i][(j | positiveMask) & negativeMask] = true;
	}
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
	const double remainingSolutions = calcBinomialCoefficientSum(karnaugh.allMinterms.size(), best.size()) * estimateRemainingSolutionsFactor(currentSolution);
	const double progressPercent = remainingSolutions / solutionSpaceSize * 100.0;
	
	std::clog << "Oops... This is taking a long time. (currently " << std::fixed << std::setprecision(1) << elapsedTime.count() << " seconds)\n";
	std::clog << "The best solution so far uses " << best.size() << " out of " << karnaugh.allMinterms.size() << " minterms. (found " << elapsedTimeSinceBest.count() << " seconds ago)\n";
	std::clog << "(Currently testing combination:";
	for (const std::size_t x : currentSolution)
		std::clog << ' ' << x;
	std::clog << ")\n";
	std::clog << "Estimation of remaining solution space: ~" << std::fixed << std::setprecision(3) << progressPercent << "% (~" << std::setprecision(0) << remainingSolutions << " combinations)\n";
	std::clog << "Press Ctrl-C to abort further search and use this solution..." << std::flush;
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
	std::clog << "\033[2K\033[A\033[2K\033[A\033[2K\033[A\033[2K\033[A\033[2K\r";
}

template<bits_t BITS>
void Karnaugh_Solution<BITS>::solve()
{
	if (karnaugh.target.none())
		return;
	
	best.resize(karnaugh.allMinterms.size());
	for (std::size_t i = 0; i != karnaugh.allMinterms.size(); ++i)
		best[i] = i;
	
	solution_t current;
	current.reserve(karnaugh.allMinterms.size());
	current.emplace_back(0);
	
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
		
		table_t currentTable;
		for (const std::size_t x : current)
			currentTable |= mintermTables[x];
		if ((currentTable & karnaugh.target) == karnaugh.target)
		{
			best = current;
			bestTime = std::chrono::steady_clock::now();
			goto go_back;
		}
		if (current.size() + 1 == best.size())
			goto go_next;
		current.emplace_back(current.back() + 1);
		if (current.back() == karnaugh.allMinterms.size())
			goto go_back;
		continue;
		go_back:
		current.pop_back();
		if (current.empty())
		{
			if (progressIntervalAdjusted)
				clearProgress();
			break;
		}
		go_next:
		if (++current.back() == karnaugh.allMinterms.size())
			goto go_back;
	}
	
	std::clog << std::flush;
}

template<bits_t BITS>
void Karnaugh_Solution<BITS>::prettyPrintBestFit() const
{
	table_t bestTable;
	for (const std::size_t x : best)
		bestTable |= mintermTables[x];
	std::cout << "best fit:\n";
	Karnaugh<BITS>::prettyPrintTable(bestTable);
}

template<bits_t BITS>
typename Karnaugh_Solution<BITS>::minterms_t Karnaugh_Solution<BITS>::getBestMinterms() const
{
	minterms_t bestMinterms;
	bestMinterms.reserve(best.size());
	for (const std::size_t x : best)
		bestMinterms.emplace_back(karnaugh.allMinterms[x]);
	return bestMinterms;
}

template<bits_t BITS>
Karnaugh_Solution<BITS> Karnaugh_Solution<BITS>::solve(const Karnaugh<BITS> &karnaugh)
{
	intSignalFlag = false;
	
	Karnaugh_Solution karnaugh_solver(karnaugh);
	karnaugh_solver.createMintermTables();
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
