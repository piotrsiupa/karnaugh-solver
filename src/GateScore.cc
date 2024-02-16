#include "./GateScore.hh"


void GateScore::printGateScores(std::ostream &o) const
{
	o << "Gate scores: NOTs = " << getNotCount() << ", ANDs = " << getAndCount() << ", ORs = " << getOrCount() << '\n';
}
