#include "./GateCost.hh"


void GateCost::printGateCost(std::ostream &o, const bool full) const
{
	const auto notCount = getNotCount();
	const auto andCount = getAndCount();
	const auto orCount = getOrCount();
	o << "Gate cost: NOTs = " << notCount << ", ANDs = " << andCount << ", ORs = " << orCount;
	if (full)
		o << ", overall cost = " << getCost(notCount, andCount, orCount)
				<< ", cost without input NOTs = " << getCost(andCount, orCount);
	o << '\n';
}
