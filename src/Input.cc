#include "./Input.hh"

#include <ios>
#include <fstream>


Minterm Input::maxMintermForMultiplying;

void Input::throwInputError(Progress *const progress)
{
	if (progress == nullptr)
		std::cerr << "Cannot read input!\n";
	else
		progress->cerr() << "Cannot read input!\n";
	throw Error("input error");
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
