#include "Implicant.hh"

#include "options.hh"


bool Implicant::humanLess(const Implicant &other) const
{
	if (const auto comparison = this->getBitCount() <=> other.getBitCount(); comparison != 0)
		return comparison < 0;
	if (const auto comparison = this->mask <=> other.mask; comparison != 0)
		return comparison > 0;
	return this->bits > other.bits;
}

Implicant::splitBits_t Implicant::splitBits() const
{
	splitBits_t splitBits;
	for (bits_t i = 0; i != ::bits; ++i)
		if ((mask & (1 << (::bits - i - 1))) != 0)
			splitBits.emplace_back(i, (bits & (1 << (::bits - i - 1))) != 0);
	return splitBits;
}

bool Implicant::isAnyInMinterms(const Minterms &minterms) const
{
	if (isEmpty() && !isEmptyTrue()) [[unlikely]]
		return false;
	const Minterm inversedMask = ~mask & ::maxMinterm;
	Minterm unmaskedPart = 0;
	do
	{
		if (minterms.check(bits | unmaskedPart))
			return true;
		unmaskedPart = (unmaskedPart - inversedMask) & inversedMask;
	} while (unmaskedPart != 0);
	return false;
}

bool Implicant::areAllInMinterms(const Minterms &minterms) const
{
	if (isEmpty() && !isEmptyTrue()) [[unlikely]]
		return true;
	const Minterm inversedMask = ~mask & ::maxMinterm;
	Minterm unmaskedPart = 0;
	do
	{
		if (!minterms.check(bits | unmaskedPart))
			return false;
		unmaskedPart = (unmaskedPart - inversedMask) & inversedMask;
	} while (unmaskedPart != 0);
	return true;
}

void Implicant::addToMinterms(Minterms &minterms) const
{
	if (isEmpty() && !isEmptyTrue()) [[unlikely]]
		return;
	const Minterm inversedMask = ~mask & ::maxMinterm;
	Minterm unmaskedPart = 0;
	do
	{
		minterms.add(bits | unmaskedPart);
		unmaskedPart = (unmaskedPart - inversedMask) & inversedMask;
	} while (unmaskedPart != 0);
}

void Implicant::removeFromMinterms(Minterms &minterms) const
{
	if (isEmpty() && !isEmptyTrue()) [[unlikely]]
		return;
	const Minterm inversedMask = ~mask & ::maxMinterm;
	Minterm unmaskedPart = 0;
	do
	{
		minterms.remove(bits | unmaskedPart);
		unmaskedPart = (unmaskedPart - inversedMask) & inversedMask;
	} while (unmaskedPart != 0);
}

void Implicant::printHuman(std::ostream &o, const bool parentheses) const
{
	if (isEmpty())
	{
		if (!isEmptyTrue())
			o << "<False>";
		else
			o << "<True>";
		return;
	}
	const bool needsParentheses = parentheses && getBitCount() != 1;
	if (needsParentheses)
		o << '(';
	bool first = true;
	for (const auto &[bitIndex, value] : splitBits())
	{
		if (first)
			first = false;
		else
			o << " && ";
		if (!value)
			o << '!';
		::inputNames.printHumanName(o, bitIndex);
	}
	if (needsParentheses)
		o << ')';
}

void Implicant::printVerilog(std::ostream &o, const bool parentheses) const
{
	if (isEmpty())
	{
		if (!isEmptyTrue())
			o << '0';
		else
			o << '1';
		return;
	}
	const bool needsParentheses = parentheses && getBitCount() != 1;
	if (needsParentheses)
		o << '(';
	bool first = true;
	for (const auto &[bitIndex, value] : splitBits())
	{
		if (first)
			first = false;
		else
			o << " & ";
		if (!value)
			o << '!';
		::inputNames.printVerilogName(o, bitIndex);
	}
	if (needsParentheses)
		o << ')';
}

void Implicant::printVhdl(std::ostream &o, const bool parentheses) const
{
	if (isEmpty())
	{
		if (!isEmptyTrue())
			o << "'0'";
		else
			o << "'1'";
		return;
	}
	const bool needsParentheses = parentheses && getBitCount() != 1;
	if (needsParentheses)
		o << '(';
	bool first = true;
	for (const auto &[bitIndex, value] : splitBits())
	{
		if (first)
			first = false;
		else
			o << " and ";
		if (!value)
			o << "not ";
		::inputNames.printVhdlName(o, bitIndex);
	}
	if (needsParentheses)
		o << ')';
}

void Implicant::printCpp(std::ostream &o, const bool parentheses) const
{
	if (isEmpty())
	{
		if (!isEmptyTrue())
			o << "false";
		else
			o << "true";
		return;
	}
	const bool needsParentheses = parentheses && getBitCount() != 1;
	if (needsParentheses)
		o << '(';
	bool first = true;
	for (const auto &[bitIndex, value] : splitBits())
	{
		if (first)
			first = false;
		else
			o << " && ";
		if (!value)
			o << "!";
		::inputNames.printCppName(o, bitIndex);
	}
	if (needsParentheses)
		o << ')';
}

void Implicant::printMath(std::ostream &o, const bool parentheses) const
{
	if (isEmpty())
	{
		if (!isEmptyTrue())
		{
			switch (options::outputFormat.getValue())
			{
			case options::OutputFormat::MATH_FORMAL:
				o << "\u22A5";
				break;
			case options::OutputFormat::MATH_ASCII:
				o << 'F';
				break;
			case options::OutputFormat::MATH_PROG:
				o << "false";
				break;
			case options::OutputFormat::MATH_NAMES:
				o << "FALSE";
				break;
			default:
				break;
			}
		}
		else
		{
			switch (options::outputFormat.getValue())
			{
			case options::OutputFormat::MATH_FORMAL:
				o << "\u22A4";
				break;
			case options::OutputFormat::MATH_ASCII:
				o << 'T';
				break;
			case options::OutputFormat::MATH_PROG:
				o << "true";
				break;
			case options::OutputFormat::MATH_NAMES:
				o << "TRUE";
				break;
			default:
				break;
			}
		}
		return;
	}
	const bool needsParentheses = parentheses && getBitCount() != 1;
	if (needsParentheses)
		o << '(';
	bool first = true;
	for (const auto &[bitIndex, value] : splitBits())
	{
		if (first)
		{
			first = false;
		}
		else
		{
			switch (options::outputFormat.getValue())
			{
			case options::OutputFormat::MATH_FORMAL:
				o << " \u2227 ";
				break;
			case options::OutputFormat::MATH_ASCII:
				o << " /\\ ";
				break;
			case options::OutputFormat::MATH_PROG:
				o << " && ";
				break;
			case options::OutputFormat::MATH_NAMES:
				o << " AND ";
				break;
			default:
				break;
			}
		}
		if (!value)
		{
			switch (options::outputFormat.getValue())
			{
			case options::OutputFormat::MATH_FORMAL:
				o << "\u00AC";
				break;
			case options::OutputFormat::MATH_ASCII:
				o << '~';
				break;
			case options::OutputFormat::MATH_PROG:
				o << '!';
				break;
			case options::OutputFormat::MATH_NAMES:
				o << "NOT ";
				break;
			default:
				break;
			}
		}
		::inputNames.printMathName(o, bitIndex);
	}
	if (needsParentheses)
		o << ')';
}
