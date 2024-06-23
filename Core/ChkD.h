#pragma once
#include <string>


class FrmlElem;

class ChkD
{
public:
	ChkD();
	ChkD(const ChkD& orig);
	FrmlElem* Bool = nullptr;
	std::string HelpName;
	FrmlElem* TxtZ = nullptr;
	bool Warning = false;
};
