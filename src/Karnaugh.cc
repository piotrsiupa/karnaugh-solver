#include "./Karnaugh.hh"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cmath>
#include <iostream>
#include <numbers>

#include "options.hh"
#include "QuineMcCluskey.hh"


std::size_t Karnaugh::nameCount = 0;

void Karnaugh::printDuplicates(const duplicates_t &duplicates, Progress::CerrGuard &cerr)
{
	bool first = true;
	for (const Minterm &minterm : duplicates)
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
			std::cout << (target.check(minterm) ? 'T' : (allowed.check(minterm) ? '-' : 'F'));
		}
		std::cout << '\n';
	}
	std::cout << std::endl;
}

void Karnaugh::prettyPrintTable() const
{
	return prettyPrintTable(*targetMinterms, *allowedMinterms);
}

void Karnaugh::prettyPrintSolution(const Implicants &solution)
{
	Minterms minterms;
	for (const auto &implicant : solution)
		implicant.addToMinterms(minterms);
	prettyPrintTable(minterms);
}

std::size_t Karnaugh::estimateRemainingInputSize(Input &input)
{
	if (!input.isFile())
		return 0;
	// The second version has a 5% margin of error baked in.
	//static constexpr double averangeNumsPerChar[::maxBits + 1] = {1.000000000000000000, 0.666666666666666630, 0.571428571428571397, 0.533333333333333326, 0.432432432432432456, 0.376470588235294112, 0.353591160220994460, 0.319201995012468820, 0.280394304490690027, 0.264326277749096561, 0.255425293090546290, 0.224340015335743242, 0.211471939697454703, 0.205576049587191639, 0.187904992373240959, 0.176649757138929470, 0.171513065780348334, 0.162541093486674615, 0.152064769530894234, 0.147317222572673323, 0.144084703848040063, 0.133865565180368712, 0.129280969725633216, 0.127104448540846543, 0.119936776396454628, 0.115355380499279264, 0.113193474308513162, 0.109025600231566433, 0.104317940109521293, 0.102113343599297607, 0.100349244075089231, 0.095396193353198613, 0.093098606926443936};
	static constexpr double averangeNumsPerChar[::maxBits + 1] = {1.000000000000000000, 0.699999999999999956, 0.599999999999999978, 0.560000000000000053, 0.454054054054054079, 0.395294117647058851, 0.371270718232044217, 0.335162094763092278, 0.294414019715224518, 0.277542591636551428, 0.268196557745073616, 0.235557016102530409, 0.222045536682327460, 0.215854852066551223, 0.197300241991903019, 0.185482244995875956, 0.180088719069365771, 0.170668148161008365, 0.159668008007438939, 0.154683083701306984, 0.151288939040442078, 0.140558843439387154, 0.135745018211914870, 0.133459670967888883, 0.125933615216277356, 0.121123149524243232, 0.118853148023938829, 0.114476880243144757, 0.109533837114997368, 0.107219010779262491, 0.105366706278843703, 0.100166003020858554, 0.097753537272766131};
	const std::size_t remainingFileSize = input.getRemainingFileSize();
	const std::size_t estimatedSize = static_cast<std::size_t>(remainingFileSize * averangeNumsPerChar[::bits]) + 10;
	return estimatedSize;
}

Progress::completion_t Karnaugh::MintermLoadingCompletionCalculator::operator()()
{
	if (inOrderSoFar)
	{
		if (currentMinterm >= lastMinterm) [[likely]]
		{
			lastMinterm = currentMinterm;
			return static_cast<Progress::completion_t>(currentMinterm) / static_cast<Progress::completion_t>(::maxMinterm);
		}
		inOrderSoFar = false;
	}
	const Progress::completion_t currentSize = static_cast<Progress::completion_t>(minterms.getSize());
	const Progress::completion_t maxSize = estimatedSize == 0
			? static_cast<Progress::completion_t>(::maxMinterm) + Progress::completion_t(1.0)
			: static_cast<Progress::completion_t>(estimatedSize);
	const Progress::completion_t expectedFraction = estimatedSize == 0 ? 0.4 : (dontCares ? 1.0 : 0.6);
	const Progress::completion_t fractionOfAll = currentSize / maxSize;
	const double scalingFactor = 1.0 - ((1.0 - expectedFraction) * std::cos(std::numbers::pi * 0.5 * fractionOfAll)); // The expected fraction gradually raises, faster and faster.
	return static_cast<Progress::completion_t>(fractionOfAll / scalingFactor);
}

std::unique_ptr<Minterms> Karnaugh::loadMinterms(Input &input, Progress &progress, const bool dontCares) const
{
	if (input.doesNextStartWithDash())
	{
		const std::string word = input.getWord();
		if (word == "-" && !input.hasNextInLine())
			return std::make_unique<Minterms>();
		Progress::cerr() << "\"-\" cannot be followed by anything!\n";
		return nullptr;
	}
	
	std::unique_ptr<Minterms> minterms(new Minterms);
	
	const std::string name = dontCares ? "don't cares" : "minterms";
	
	const std::string info = "loading " + name;
	const auto infoGuard = progress.addInfo(info.c_str());
	progress.step(true);
	Minterm currentMinterm;
	const std::size_t estimatedSize = estimateRemainingInputSize(input);
	const Progress::calcStepCompletion_t calcStepCompletion(MintermLoadingCompletionCalculator(*minterms, currentMinterm, dontCares, estimatedSize));
	duplicates_t duplicates;
	do
	{
		progress.substep(calcStepCompletion);
		currentMinterm = input.getMinterm();
		if (!minterms->add(currentMinterm))
			duplicates.push_back(currentMinterm);
	} while (input.hasNextInLine());
	if (!duplicates.empty())
	{
		Progress::CerrGuard cerr = Progress::cerr();
		cerr << "There are duplicates on the " << name << " list:";
		printDuplicates(duplicates, cerr);
		cerr << "! (They will be ignored.)\n";
	}
	
	return minterms;
}

#ifndef NDEBUG
void Karnaugh::validate(const solutions_t &solutions) const
{
	const std::string progressName = "Validating solutions for \"" + functionName + "\" (development build)";
	Progress progress(Progress::Stage::SOLVING, progressName.c_str(), solutions.size(), true);
	for (const Implicants &solution : solutions)
	{
		progress.step();
		for (Minterm i = 0;; ++i)
		{
			progress.substep([i = std::as_const(i)](){ return static_cast<Progress::completion_t>(i) / (static_cast<Progress::completion_t>(::maxMinterm) + 1.0); });
			if (targetMinterms->check(i))
				assert(solution.covers(i));
			else if (!allowedMinterms->check(i))
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
	Progress progress(Progress::Stage::LOADING, progressName.c_str(), 3, false, !options::prompt.getValue());
	
	nameIsCustom = input.isNextText();
	if (nameIsCustom)
		functionName = input.getLine();
	
	if (nameIsCustom && options::prompt.getValue())
		std::cerr << "Enter a list of minterms of the function \"" << functionName << "\":\n";
	if (!input.hasNext())
	{
		Progress::cerr() << "A list of minterms is mandatory!\n";
		return false;
	}
	if (targetMinterms = loadMinterms(input, progress, false); !targetMinterms)
		return false;
	
	if (options::prompt.getValue())
	{
		std::cerr << "Enter a list of don't-cares of the function";
		if (nameIsCustom)
			std::cerr << " \"" << functionName << "\":\n";
		else
			std::cerr << ":\n";
	}
	if (input.hasNext())
		if (allowedMinterms = loadMinterms(input, progress, true); !allowedMinterms)
			return false;

#ifndef NDEBUG
	targetMinterms->validate();
	allowedMinterms->validate();
#endif
	
	{
		const auto infoGuard = progress.addInfo("listing possible minterms");
		progress.step(true);
		progress.substep([](){ return -0.0; }, true);
		const duplicates_t duplicates = allowedMinterms->findDuplicates(*targetMinterms);
		if (!duplicates.empty())
		{
			Progress::CerrGuard cerr = Progress::cerr();
			cerr << "There are numbers in \"don't cares\" that are already minterms:";
			printDuplicates(duplicates, cerr);
			cerr << "! (They will be ignored.)\n";
		}
		progress.substep([](){ return -0.5; }, true);
		allowedMinterms->add(*targetMinterms, duplicates.size());
	}

#ifndef NDEBUG
	allowedMinterms->validate();
#endif
	
	return true;
}

Karnaugh::solutions_t Karnaugh::solve() const
{
#ifdef NDEBUG
	const bool mintermsWillBeNeededLater = isTableSmallEnoughToPrint();
#else
	constexpr bool mintermsWillBeNeededLater = true;
#endif
	const Karnaugh::solutions_t solutions = mintermsWillBeNeededLater
			? QuineMcCluskey(functionName, allowedMinterms, targetMinterms).solve()
			: QuineMcCluskey(functionName, std::move(allowedMinterms), std::move(targetMinterms)).solve();
#ifndef NDEBUG
	validate(solutions);
#endif
	return solutions;
}

void Karnaugh::printHumanSolution(const Implicants &solution) const
{
	if (options::outputFormat.getValue() == options::OutputFormat::HUMAN_LONG)
	{
		if (isTableSmallEnoughToPrint())
		{
			std::cout << "goal:\n";
			prettyPrintTable();
			
			if (*targetMinterms != *allowedMinterms)
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
	Implicants(solution).humanSort().printHuman(std::cout);
	if (options::outputFormat.getValue() != options::OutputFormat::HUMAN_SHORT)
		std::cout << '\n';
}

void Karnaugh::printVerilogSolution(const Implicants &solution) const
{
	Implicants(solution).humanSort().printVerilog(std::cout);
}

void Karnaugh::printVhdlSolution(const Implicants &solution) const
{
	Implicants(solution).humanSort().printVhdl(std::cout);
}

void Karnaugh::printCppSolution(const Implicants &solution) const
{
	Implicants(solution).humanSort().printCpp(std::cout);
}

void Karnaugh::printMathSolution(const Implicants &solution) const
{
	Implicants(solution).humanSort().printMath(std::cout);
}
