#include "./Karnaugh.hh"

#include <cassert>
#include <cctype>
#include <iostream>

#include "options.hh"
#include "QuineMcCluskey.hh"


std::size_t Karnaugh::nameCount = 0;

bool Karnaugh::loadMinterms(Minterms &minterms, Input &input, Progress &progress) const
{
	std::vector<std::string> parts;
	{
		const auto infoGuard = progress.addInfo("splitting input line");
		progress.step(true);
		parts = input.popParts(progress);
	}
	
	{
		const auto infoGuard = progress.addInfo("parsing numbers");
		progress.step(true);
		auto progressStep = progress.makeCountingStepHelper(static_cast<Progress::completion_t>(parts.size()));
		for (const std::string &string : parts)
		{
			progressStep.substep();
			try
			{
				const unsigned long n = std::stoul(string);
				static_assert(sizeof(unsigned long) * CHAR_BIT >= ::maxBits);
				if (n > ::maxMinterm)
				{
					Progress::cerr() << '"' << string << "\" is too big!\n";
					return false;
				}
				minterms.insert(n);
			}
			catch (std::invalid_argument &)
			{
				Progress::cerr() << '"' << string << "\" is not a number!\n";
				return false;
			}
			catch (std::out_of_range &)
			{
				Progress::cerr() << '"' << string << "\" is out of range!\n";
				return false;
			}
		}
	}
	return true;
}

#ifndef NDEBUG
void Karnaugh::validate(const Solutions &solutions) const
{
	const std::string progressName = "Validating solutions for \"" + functionName + "\" (development build)";
	Progress progress(Progress::Stage::SOLVING, progressName.c_str(), solutions.size());
	for (const Solution &solution : solutions)
	{
		progress.step();
		for (Minterm i = 0;; ++i)
		{
			progress.substep([i = std::as_const(i)](){ return static_cast<Progress::completion_t>(i) / (static_cast<Progress::completion_t>(::maxMinterm) + 1.0); });
			if (targetMinterms.find(i) != targetMinterms.cend())
				assert(solution.covers(i));
			else if (allowedMinterms.find(i) == allowedMinterms.cend())
				assert(!solution.covers(i));
			if (i == ::maxMinterm)
				break;
		}
	}
}
#endif

bool Karnaugh::loadData(Input &input)
{
	const std::string progressName = "Loading function \"" + functionName + '"';
	Progress progress(Progress::Stage::LOADING, progressName.c_str(), 5, false, !options::prompt);
	
	if (input.hasError(&progress))
		return false;
	nameIsCustom = input.isName();
	if (nameIsCustom)
		functionName = input.popLine();
	
	if (nameIsCustom && options::prompt)
		std::cerr << "Enter a list of minterms of the function \"" << functionName << "\":\n";
	if (input.hasError(&progress))
		return false;
	if (input.isEmpty())
	{
		Progress::cerr() << "A list of minterms is mandatory!\n";
		return false;
	}
	if (!loadMinterms(targetMinterms, input, progress))
		return false;
	
	if (options::prompt)
	{
		std::cerr << "Enter a list of don't-cares of the function";
		if (nameIsCustom)
			std::cerr << " \"" << functionName << "\":\n";
		else
			std::cerr << ":\n";
	}
	if (input.hasError(&progress))
		return false;
	if (!input.isEmpty())
		if (!loadMinterms(allowedMinterms, input, progress))
			return false;
	
	const auto infoGuard = progress.addInfo("checking for conflicts");
	progress.step(true);
	auto progressStep = progress.makeCountingStepHelper(static_cast<Progress::completion_t>(targetMinterms.size()));
	for (const Minterm &targetMinterm : targetMinterms)
	{
		progressStep.substep();
		if (allowedMinterms.find(targetMinterm) == allowedMinterms.cend())
			allowedMinterms.insert(targetMinterm);
		else
			Progress::cerr() << targetMinterm << " on the \"don't care\" list of \"" << functionName << "\" will be ignored because it is already a minterm!\n";
	}
	
	return true;
}

Solutions Karnaugh::solve() const
{
	const Solutions solutions = QuineMcCluskey().solve(allowedMinterms, targetMinterms, functionName);
#ifndef NDEBUG
	validate(solutions);
#endif
	return solutions;
}
