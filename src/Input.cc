#include "./Input.hh"

#include <ios>
#include <fstream>

#include "options.hh"


Minterm Input::maxMintermForMultiplying;

void Input::throwInputError()
{
	Progress::cerr() << "Cannot read input!\n";
	throw Error("input error");
}

bool Input::refillBuffer()
{
	if (options::prompt.getValue()) [[unlikely]]
	{
		istream.read(buffer, 1);
		if (istream) [[likely]]
			bufferSize = static_cast<std::uint16_t>(istream.readsome(buffer + 1, bufferCapacity - 1)) + 1;
		else if (istream.eof()) [[likely]]
			return false;
		if (!istream) [[unlikely]]
		{
			if (!istream.eof()) [[unlikely]]
				throwInputError();
			istream.clear();
		}
		else if (bufferSize == 0) [[unlikely]]
		{
			return false;
		}
	}
	else
	{
		istream.read(buffer, bufferCapacity);
		if (!istream) [[unlikely]]
		{
			if (!istream.eof()) [[unlikely]]
				throwInputError();
			bufferSize = static_cast<std::uint16_t>(istream.gcount());
			if (bufferSize == 0)
				return false;
			istream.clear();
		}
	}
	bufferPos = 0;
	return true;
}

bool Input::isFile() const
{
	return dynamic_cast<std::ifstream*>(&istream) != nullptr;
}

std::size_t Input::getRemainingFileSize()
{
	const auto currentPos = istream.tellg();
	istream.seekg(0, std::ios_base::end);
	const auto endPos = istream.tellg();
	istream.seekg(currentPos);
	return (endPos - currentPos) + (bufferSize - bufferPos) + 1;
}
