#pragma once
#include <string>
#include "base.h"
#include "Chained.h"

class FrmlElem;

class ChkD : public Chained<ChkD> // ø. 115
{
public:
	ChkD() {};
	ChkD(const ChkD& orig);
	// ChkD* pChain = nullptr;
	FrmlElem* Bool = nullptr;
	std::string HelpName;
	FrmlElem* TxtZ = nullptr;
	bool Warning = false;
};
typedef ChkD* ChkDPtr;
