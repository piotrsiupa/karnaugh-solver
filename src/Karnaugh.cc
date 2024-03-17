#include "./Karnaugh.hh"

#include <cassert>
#include <cctype>
#include <iostream>

#include "options.hh"
#include "QuineMcCluskey.hh"


std::size_t Karnaugh::nameCount = 0;

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
			std::cout << (target.find(minterm) != target.cend() ? 'T' : (allowed.find(minterm) != allowed.cend() ? '-' : 'F'));
		}
		std::cout << '\n';
	}
	std::cout << std::endl;
}

void Karnaugh::prettyPrintTable() const
{
	return prettyPrintTable(targetMinterms, allowedMinterms);
}

void Karnaugh::prettyPrintSolution(const Solution &solution)
{
	Minterms minterms;
	for (const auto &implicant : solution)
	{
		const auto newMinterms = implicant.findMinterms();
		minterms.insert(newMinterms.cbegin(), newMinterms.end());
	}
	prettyPrintTable(minterms);
}

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
	Progress progress(Progress::Stage::LOADING, progressName.c_str(), 5, false, !options::prompt.getValue());
	
	if (input.hasError(&progress))
		return false;
	nameIsCustom = input.isName();
	if (nameIsCustom)
		functionName = input.popLine();
	
	if (nameIsCustom && options::prompt.getValue())
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
	
	if (options::prompt.getValue())
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

void Karnaugh::printHumanSolution(const Solution &solution) const
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
	Solution(solution).sort().printHuman(std::cout);
}

std::size_t Karnaugh::printGraphSolution(const Solution &solution, const std::size_t functionNum, std::size_t idShift) const
{
	std::cout << "\tsubgraph function_" << functionNum << '\n';
	std::cout << "\t{\n";
	idShift = Solution(solution).sort().printGraph(std::cout, functionNum, functionName, idShift);
	std::cout << "\t}\n";
	return idShift;
}

void Karnaugh::printVerilogSolution(const Solution &solution) const
{
	Solution(solution).sort().printVerilog(std::cout);
}

void Karnaugh::printVhdlSolution(const Solution &solution) const
{
	Solution(solution).sort().printVhdl(std::cout);
}

void Karnaugh::printCppSolution(const Solution &solution) const
{
	Solution(solution).sort().printCpp(std::cout);
}

void Karnaugh::printMathSolution(const Solution &solution) const
{
	Solution(solution).sort().printMath(std::cout);
}
