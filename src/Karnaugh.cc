#include "./Karnaugh.hh"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <iostream>

#include "options.hh"
#include "QuineMcCluskey.hh"


std::size_t Karnaugh::nameCount = 0;

Minterms Karnaugh::extractDuplicates(Minterms &minterms)
{
	if (minterms.empty())
        return {};
	
	const Minterms::const_iterator last = minterms.end();
	
    Minterms::iterator current = minterms.begin();
    while (true)
	{
		const Minterms::iterator next = std::next(current);
		if (next == last)
			return {};
		if (*current == *next)
			break;
		current = next;
	}
	
	Minterms duplicates;
	Minterms::iterator result = current;
	while (true)
	{
		const Minterms::iterator next = std::next(current);
		if (next == last)
			break;
		if (*current == *next)
			duplicates.emplace_back(std::move(*next));
		else
			*++result = std::move(*next);
		current = next;
	}
	minterms.erase(std::next(result), last);
    return duplicates;
}

void Karnaugh::printMinterms(const Minterms &minterms, Progress::CerrGuard &cerr)
{
	bool first = true;
	for (const Minterm &minterm : minterms)
	{
		if (first)
			first = false;
		else
			cerr << ',';
		cerr << ' ' << minterm;
	}
}

Karnaugh::grayCode_t Karnaugh::makeGrayCode(const bits_t bitCount)
{
	grayCode_t grayCode;
	grayCode.reserve(static_cast<unsigned>(1u << bitCount));
	grayCode.push_back(0);
	if (bitCount != 0)
	{
		grayCode.push_back(1);
		for (bits_t i = 1; i != bitCount; ++i)
			for (Minterm j = 0; j != unsigned(1) << i; ++j)
				grayCode.push_back(grayCode[j ^ ((1 << i) - 1)] | (1 << i));
	}
	return grayCode;
}

void Karnaugh::printBits(const Minterm minterm, const bits_t bitCount)
{
	for (bits_t i = bitCount; i != 0; --i)
		std::cout << ((minterm & (1 << (i - 1))) != 0 ? '1' : '0');
}

void Karnaugh::prettyPrintTable(const Minterms &target, const Minterms &allowed)
{
	const bits_t vBits = (::bits + 1) / 2;
	const bits_t hBits = ::bits / 2;
	const grayCode_t vGrayCode = makeGrayCode(vBits);
	const grayCode_t hGrayCode = makeGrayCode(hBits);
	for (int i = 0; i != vBits; ++i)
		std::cout << ' ';
	std::cout << ' ';
	for (const Minterm y : hGrayCode)
	{
		printBits(y, hBits);
		std::cout << ' ';
	}
	std::cout << '\n';
	for (const Minterm x : vGrayCode)
	{
		printBits(x, std::max(bits_t(1), vBits));
		std::cout << ' ';
		bool first = true;
		for (int i = 0; i != (hBits - 1) / 2; ++i)
			std::cout << ' ';
		for (const Minterm y : hGrayCode)
		{
			if (first)
				first = false;
			else
				for (int i = 0; i != hBits; ++i)
					std::cout << ' ';
			const Minterm minterm = (x << hBits) | y;
			std::cout << (std::find(target.cbegin(), target.cend(), minterm) != target.cend() ? 'T' : (std::find(allowed.cbegin(), allowed.cend(), minterm) != allowed.cend() ? '-' : 'F'));
		}
		std::cout << '\n';
	}
	std::cout << std::endl;
}

void Karnaugh::prettyPrintTable() const
{
	return prettyPrintTable(targetMinterms, allowedMinterms);
}

void Karnaugh::prettyPrintSolution(const Implicants &solution)
{
	Minterms minterms;
	for (const auto &implicant : solution)
	{
		const auto newMinterms = implicant.findMinterms();
		minterms.insert(minterms.end(), newMinterms.begin(), newMinterms.end());
	}
	prettyPrintTable(minterms);
}

bool Karnaugh::loadMinterms(Minterms &minterms, Input &input, Progress &progress, const std::string &name) const
{
	if (input.doesNextStartWithDash())
	{
		const std::string word = input.getWord(&progress);
		if (word == "-" && !input.hasNextInLine(&progress))
			return true;
		progress.cerr() << "\"-\" cannot be followed by anything!\n";
		return false;
	}
	
	{
		const std::string subtaskName = "loading " + name;
		const auto subtaskGuard = progress.enterSubtask(subtaskName.c_str());
		progress.step(true);
		Minterm lastMinterm = 0;
		const Progress::calcSubstepCompletion_t calcSubstepCompletion = [&lastMinterm = std::as_const(lastMinterm)](){ return static_cast<Progress::completion_t>(lastMinterm) / static_cast<Progress::completion_t>(::maxMinterm); };
		do
		{
			progress.substep(calcSubstepCompletion);
			lastMinterm = input.getMinterm(progress);
			minterms.push_back(lastMinterm);
		} while (input.hasNextInLine(&progress));
	}
	
	{
		const std::string subtaskName = "sorting " + name + " (*)";
		const auto subtaskGuard = progress.enterSubtask(subtaskName.c_str());
		progress.step(true);
		progress.substep([](){ return 0.0; }, true);
		std::sort(minterms.begin(), minterms.end());
		progress.substep([](){ return 0.8; }, true);
		const Minterms duplicates = extractDuplicates(minterms);
		if (!duplicates.empty())
		{
			Progress::CerrGuard cerr = progress.cerr();
			cerr << "There are duplicates on the " << name << " list:";
			printMinterms(duplicates, cerr);
			cerr << "! (They will be ignored.)\n";
		}
		minterms.shrink_to_fit();
	}
	return true;
}

#ifndef NDEBUG
void Karnaugh::validate(const solutions_t &solutions) const
{
	const std::string progressName = "Validating solutions for \"" + functionName + "\" (development build)";
	Progress progress(Progress::Stage::SOLVING, progressName.c_str(), solutions.size());
	for (const Implicants &solution : solutions)
	{
		progress.step();
		for (Minterm i = 0;; ++i)
		{
			progress.substep([i = std::as_const(i)](){ return static_cast<Progress::completion_t>(i) / (static_cast<Progress::completion_t>(::maxMinterm) + 1.0); });
			if (std::find(targetMinterms.cbegin(), targetMinterms.cend(), i) != targetMinterms.cend())
				assert(solution.covers(i));
			else if (std::find(allowedMinterms.cbegin(), allowedMinterms.cend(), i) == allowedMinterms.cend())
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
	Progress progress(Progress::Stage::LOADING, progressName.c_str(), 5, !options::prompt.getValue());
	
	nameIsCustom = input.isNextText();
	if (nameIsCustom)
		functionName = input.getLine(&progress);
	
	if (nameIsCustom && options::prompt.getValue())
		std::cerr << "Enter a list of minterms of the function \"" << functionName << "\":\n";
	if (!input.hasNext(&progress))
	{
		progress.cerr() << "A list of minterms is mandatory!\n";
		return false;
	}
	if (!loadMinterms(targetMinterms, input, progress, "minterms"))
		return false;
	
	if (options::prompt.getValue())
	{
		std::cerr << "Enter a list of don't-cares of the function";
		if (nameIsCustom)
			std::cerr << " \"" << functionName << "\":\n";
		else
			std::cerr << ":\n";
	}
	if (input.hasNext(&progress))
		if (!loadMinterms(allowedMinterms, input, progress, "don't cares"))
			return false;
	
	{
		const auto conflictsSubtask = progress.enterSubtask("listing possible minterms (*)");
		progress.step(true);
		progress.substep([](){ return 0.0; }, true);
		allowedMinterms.reserve(targetMinterms.size() + allowedMinterms.size());
		allowedMinterms.insert(allowedMinterms.end(), targetMinterms.cbegin(), targetMinterms.cend());
		progress.substep([](){ return 0.1; }, true);
		std::sort(allowedMinterms.begin(), allowedMinterms.end());
		progress.substep([](){ return 0.8; }, true);
		const Minterms duplicates = extractDuplicates(allowedMinterms);
		if (!duplicates.empty())
		{
			Progress::CerrGuard cerr = progress.cerr();
			cerr << "There are numbers in \"don't cares\" that are already minterms:";
			printMinterms(duplicates, cerr);
			cerr << "! (They will be ignored.)\n";
		}
	}
	
	return true;
}

Karnaugh::solutions_t Karnaugh::solve() const
{
	const Karnaugh::solutions_t solutions = QuineMcCluskey().solve(allowedMinterms, targetMinterms, functionName);
#ifndef NDEBUG
	validate(solutions);
#endif
	return solutions;
}

void Karnaugh::printHumanSolution(const Implicants &solution) const
{
	if (options::outputFormat.getValue() == options::OutputFormat::HUMAN_LONG)
	{
		if (::bits <= 8)
		{
			std::cout << "goal:\n";
			prettyPrintTable();
			
			if (targetMinterms.size() != allowedMinterms.size())
			{
				std::cout << "best fit:\n";
				prettyPrintSolution(solution);
			}
		}
		else
		{
			std::cout << "The Karnaugh map is too big to be displayed.\n\n";
		}
		std::cout << "solution:\n";
	}
	Implicants(solution).sort().printHuman(std::cout);
	if (options::outputFormat.getValue() != options::OutputFormat::HUMAN_SHORT)
		std::cout << '\n';
}

void Karnaugh::printVerilogSolution(const Implicants &solution) const
{
	Implicants(solution).sort().printVerilog(std::cout);
}

void Karnaugh::printVhdlSolution(const Implicants &solution) const
{
	Implicants(solution).sort().printVhdl(std::cout);
}

void Karnaugh::printCppSolution(const Implicants &solution) const
{
	Implicants(solution).sort().printCpp(std::cout);
}

void Karnaugh::printMathSolution(const Implicants &solution) const
{
	Implicants(solution).sort().printMath(std::cout);
}
