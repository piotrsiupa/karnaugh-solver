#include "Implicant.hh"

#include "options.hh"
#include "utils.hh"


bool Implicant::operator<(const Implicant &other) const
{
	if (this->bitCount != other.bitCount)
		return this->bitCount < other.bitCount;
	const mask_t thisMask = this->trueBits | this->falseBits;
	const mask_t otherMask = other.trueBits | other.falseBits;
	if (thisMask != otherMask)
		return thisMask > otherMask;
	return this->trueBits > other.trueBits;
}

Implicant::splitBits_t Implicant::splitBits() const
{
	splitBits_t splitBits;
	for (bits_t i = 0; i != ::bits; ++i)
	{
		const bool trueBit = (trueBits & (1 << (::bits - i - 1))) != 0;
		const bool falseBit = (falseBits & (1 << (::bits - i - 1))) != 0;
		if (trueBit || falseBit)
			splitBits.emplace_back(i, falseBit);
	}
	return splitBits;
}

Implicant::minterms_t Implicant::findMinterms() const
{
	minterms_t minterms;
	for (Minterm minterm = 0; minterm != ::maxMinterm; ++minterm)
		if (covers(minterm))
			minterms.push_back(minterm);
	if (covers(maxMinterm)) // This is not handled by the loop in case `::maxMinterm` is really the max value of underlying type of `Minterm`.
		minterms.push_back(maxMinterm);
	return minterms;
}

bool Implicant::areMergeable(const Implicant &x, const Implicant &y)
{
	if (x.bitCount != y.bitCount)
		return false;
	const mask_t trueBitsDiff = x.trueBits ^ y.trueBits;
	const mask_t falseBitsDiff = x.falseBits ^ y.falseBits;
	if (trueBitsDiff != falseBitsDiff)
		return false;
	return std::bitset<32>(trueBitsDiff).count() == 1;
}

Implicant Implicant::merge(const Implicant &x, const Implicant &y)
{
	return Implicant(x.trueBits & y.trueBits, x.falseBits & y.falseBits, x.bitCount - 1);
}

void Implicant::printHuman(std::ostream &o, const bool parentheses) const
{
	if (bitCount == 0)
	{
		if (isError())
			o << "<False>";
		else
			o << "<True>";
		return;
	}
	const bool needsParentheses = parentheses && bitCount != 1;
	if (needsParentheses)
		o << '(';
	First first;
	for (const auto &[bitIndex, negated] : splitBits())
	{
		if (!first)
			o << " && ";
		if (negated)
			o << '!';
		::inputNames.printHumanName(o, bitIndex);
	}
	if (needsParentheses)
		o << ')';
}

void Implicant::printGraph(std::ostream &o) const
{
	if (bitCount == 0)
	{
		if (isError())
			o << "false";
		else
			o << "true";
		return;
	}
	First first;
	for (const auto &[bitIndex, negated] : splitBits())
	{
		if (!first)
			o << ", ";
		if (negated)
			o << 'n';
		o << 'i' << static_cast<unsigned>(bitIndex);
	}
}

void Implicant::printVerilog(std::ostream &o, const bool parentheses) const
{
	if (bitCount == 0)
	{
		if (isError())
			o << '0';
		else
			o << '1';
		return;
	}
	const bool needsParentheses = parentheses && bitCount != 1;
	if (needsParentheses)
		o << '(';
	First first;
	for (const auto &[bitIndex, negated] : splitBits())
	{
		if (!first)
			o << " & ";
		if (negated)
			o << '!';
		::inputNames.printVerilogName(o, bitIndex);
	}
	if (needsParentheses)
		o << ')';
}

void Implicant::printVhdl(std::ostream &o, const bool parentheses) const
{
	if (bitCount == 0)
	{
		if (isError())
			o << "'0'";
		else
			o << "'1'";
		return;
	}
	const bool needsParentheses = parentheses && bitCount != 1;
	if (needsParentheses)
		o << '(';
	First first;
	for (const auto &[bitIndex, negated] : splitBits())
	{
		if (!first)
			o << " and ";
		if (negated)
			o << "not ";
		::inputNames.printVhdlName(o, bitIndex);
	}
	if (needsParentheses)
		o << ')';
}

void Implicant::printCpp(std::ostream &o, const bool parentheses) const
{
	if (bitCount == 0)
	{
		if (isError())
			o << "false";
		else
			o << "true";
		return;
	}
	const bool needsParentheses = parentheses && bitCount != 1;
	if (needsParentheses)
		o << '(';
	First first;
	for (const auto &[bitIndex, negated] : splitBits())
	{
		if (!first)
			o << " && ";
		if (negated)
			o << "!";
		::inputNames.printCppName(o, bitIndex);
	}
	if (needsParentheses)
		o << ')';
}

void Implicant::printMath(std::ostream &o, const bool parentheses) const
{
	if (bitCount == 0)
	{
		if (isError())
		{
			switch (options::outputFormat.getValue())
			{
			case options::OutputFormat::MATH_FORMAL:
				o << u8"\u22A5";
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
				o << u8"\u22A4";
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
	const bool needsParentheses = parentheses && bitCount != 1;
	if (needsParentheses)
		o << '(';
	First first;
	for (const auto &[bitIndex, negated] : splitBits())
	{
		if (!first)
		{
			switch (options::outputFormat.getValue())
			{
			case options::OutputFormat::MATH_FORMAL:
				o << u8" \u2227 ";
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
		if (negated)
		{
			switch (options::outputFormat.getValue())
			{
			case options::OutputFormat::MATH_FORMAL:
				o << u8"\u00AC";
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
