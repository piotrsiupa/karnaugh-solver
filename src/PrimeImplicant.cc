#include "PrimeImplicant.hh"


void PrimeImplicant::recalculateBits()
{
	bitCount = __builtin_popcount(trueBits | falseBits);
}

PrimeImplicant::splitBits_t PrimeImplicant::splitBits(const bits_t bits) const
{
	splitBits_t splitBits;
	for (bits_t i = 0; i != bits; ++i)
	{
		const bool trueBit = (trueBits & (1 << (bits - i - 1))) != 0;
		const bool falseBit = (falseBits & (1 << (bits - i - 1))) != 0;
		if (trueBit || falseBit)
			splitBits.emplace_back(i, falseBit);
	}
	return splitBits;
}

bool PrimeImplicant::operator<(const PrimeImplicant &other) const
{
	if (this->bitCount != other.bitCount)
		return this->bitCount < other.bitCount;
	const mask_t thisMask = this->trueBits | this->falseBits;
	const mask_t otherMask = other.trueBits | other.falseBits;
	if (thisMask != otherMask)
		return thisMask > otherMask;
	return this->trueBits > other.trueBits;
}

PrimeImplicant::minterms_t PrimeImplicant::findMinterms(const bits_t bits) const
{
	minterms_t minterms;
	static_assert(sizeof(Minterm) * CHAR_BIT > maxBits);
	for (Minterm minterm = 0; minterm != Minterm(1) << bits; ++minterm)
		if (covers(minterm))
			minterms.push_back(minterm);
	return minterms;
}

bool PrimeImplicant::areMergeable(const PrimeImplicant &x, const PrimeImplicant &y)
{
	if (x.bitCount != y.bitCount)
		return false;
	const mask_t trueBitsDiff = x.trueBits ^ y.trueBits;
	const mask_t falseBitsDiff = x.falseBits ^ y.falseBits;
	if (trueBitsDiff != falseBitsDiff)
		return false;
	return __builtin_popcount(trueBitsDiff) == 1;
}

PrimeImplicant PrimeImplicant::merge(const PrimeImplicant &x, const PrimeImplicant &y)
{
	return PrimeImplicant(x.trueBits & y.trueBits, x.falseBits & y.falseBits, x.bitCount - 1);
}

void PrimeImplicant::print(std::ostream &o, const bits_t bits, const names_t &inputNames, const bool parentheses) const
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
	for (const auto &splitBit : splitBits(bits))
	{
		if (first)
			first = false;
		else
			o << " && ";
		if (splitBit.second)
			o << '!';
		o << inputNames[splitBit.first];
	}
	if (needsParentheses)
		o << ')';
}
