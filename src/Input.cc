#include "./Input.hh"



Minterm Input::maxMintermForMultiplying;

void Input::throwInputError(Progress *const progress)
{
	if (progress == nullptr)
		std::cerr << "Cannot read input!\n";
	else
		progress->cerr() << "Cannot read input!\n";
	throw Error("input error");
}
