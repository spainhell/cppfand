#pragma once
#include <string>
#include "base.h"

class FrmlElem;

class ChkD : public Chained // ø. 115
{
public:
	ChkD() {};
	ChkD(const ChkD& orig);
	// ChkD* Chain = nullptr;
	FrmlElem* Bool = nullptr;
	std::string* HelpName = nullptr;
	FrmlElem* TxtZ = nullptr;
	bool Warning = false;
};
typedef ChkD* ChkDPtr;
