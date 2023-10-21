#include "./Karnaugh.hh"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <iostream>

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

void Karnaugh::prettyPrintSolution(const PrimeImplicants &solution)
{
	Minterms minterms;
	for (const auto &primeImplicant : solution)
	{
		const auto newMinterms = primeImplicant.findMinterms();
		minterms.insert(newMinterms.cbegin(), newMinterms.end());
	}
	prettyPrintTable(minterms);
}

void Karnaugh::printPrimeImplicant(const PrimeImplicant primeImplicant, const bool parentheses) const
{
	return primeImplicant.print(std::cout, parentheses);
}

void Karnaugh::printPrimeImplicants(PrimeImplicants primeImplicants) const
{
	if (primeImplicants.size() == 1)
	{
		printPrimeImplicant(primeImplicants.front(), false);
	}
	else
	{
		std::sort(primeImplicants.begin(), primeImplicants.end());
		for (const PrimeImplicant &primeImplicant : primeImplicants)
		{
			if (&primeImplicant != &primeImplicants.front())
				std::cout << " || ";
			printPrimeImplicant(primeImplicant, true);
		}
	}
}

bool Karnaugh::loadMinterms(Minterms &minterms, Input &input) const
{
	for (const std::string &string : input.popParts())
	{
		try
		{
			const unsigned long n = std::stoul(string);
			static_assert(sizeof(unsigned long) * CHAR_BIT >= ::maxBits);
			if (n > ::maxMinterm)
			{
				std::cerr << '"' << string << "\" is too big!\n";
				return false;
			}
			minterms.insert(n);
		}
		catch (std::invalid_argument &)
		{
			std::cerr << '"' << string << "\" is not a number!\n";
			return false;
		}
		catch (std::out_of_range &)
		{
			std::cerr << '"' << string << "\" is out of range!\n";
			return false;
		}
	}
	return true;
}

#ifndef NDEBUG
void Karnaugh::validate(const solutions_t &solutions) const
{
	for (Minterm i = 0;; ++i)
	{
		if (targetMinterms.find(i) != targetMinterms.cend())
			for (const PrimeImplicants &solution : solutions)
				assert(solution.covers(i));
		else if (allowedMinterms.find(i) == allowedMinterms.cend())
			for (const PrimeImplicants &solution : solutions)
				assert(!solution.covers(i));
		if (i == ::maxMinterm)
			break;
	}
}
#endif

bool Karnaugh::loadData(Input &input)
{
	if (input.hasError())
		return false;
	const bool hasName = input.isName();
	if (hasName)
		functionName = input.popLine();
	
	
	if (hasName && ::terminalStdin)
		std::cerr << "Enter a list of minterms of the function \"" << functionName << "\":\n";
	if (input.hasError())
		return false;
	if (input.isEmpty())
	{
		std::cerr << "A list of minterms is mandatory!\n";
		return false;
	}
	if (!loadMinterms(targetMinterms, input))
		return false;
	
	if (::terminalStdin)
	{
		std::cerr << "Enter a list of don't-cares of the function";
		if (hasName)
			std::cerr << " \"" << functionName << "\":\n";
		else
			std::cerr << ":\n";
	}
	if (input.hasError())
		return false;
	if (!input.isEmpty())
		if (!loadMinterms(allowedMinterms, input))
			return false;
	
	for (const Minterm &targetMinterm : targetMinterms)
	{
		if (allowedMinterms.find(targetMinterm) == allowedMinterms.cend())
			allowedMinterms.insert(targetMinterm);
		else
			std::cerr << targetMinterm << " on the \"don't care\" list of \"" << functionName << "\" will be ignored because it is already a minterm!\n";
	}
	
	return true;
}

Karnaugh::solutions_t Karnaugh::solve() const
{
	if (::terminalStderr)
		std::clog << "Searching for solutions for the function \"" << functionName << "\"..." << std::endl;
	const Karnaugh::solutions_t solutions = QuineMcCluskey().solve(allowedMinterms, targetMinterms);
#ifndef NDEBUG
	if (::terminalStderr)
		std::clog << "Validating the found solutions for the function \"" << functionName << "\" (development build)..." << std::endl;
	validate(solutions);
#endif
	return solutions;
}

void Karnaugh::printSolution(const PrimeImplicants &solution) const
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
	printPrimeImplicants(solution);
	std::cout << std::endl;
}
