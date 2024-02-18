#pragma once

#include <vector>

#include "Implicant.hh"


class Implicants : public std::vector<Implicant>
{
public:
	using std::vector<Implicant>::vector;
	Implicants(std::vector<Implicant> &&implicants) : std::vector<Implicant>(std::move(implicants)) {}
	
	Implicants& humanSort();
};
