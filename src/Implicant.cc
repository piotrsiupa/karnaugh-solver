#include "Implicant.hh"


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
	bool first = true;
	for (const auto &[bitIndex, negated] : splitBits())
	{
		if (first)
			first = false;
		else
			o << " && ";
		if (negated)
			o << '!';
		::inputNames.printHumanName(o, bitIndex);
	}
	if (needsParentheses)
		o << ')';
}

void Implicant::printVerilog(std::ostream &o, const bool parentheses) const
{
	if (bitCount == 0)
	{
		if (isError())
			o << "0";
		else
			o << "1";
		return;
	}
	const bool needsParentheses = parentheses && bitCount != 1;
	if (needsParentheses)
		o << '(';
	bool first = true;
	for (const auto &[bitIndex, negated] : splitBits())
	{
		if (first)
			first = false;
		else
			o << " & ";
		if (negated)
			o << '!';
		::inputNames.printVerilogName(o, bitIndex);
	}
	if (needsParentheses)
		o << ')';
}
