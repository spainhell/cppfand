#pragma once
#include <vector>

#include "OutpFD.h"

struct AssignD;
class FrmlElem;

struct OutpRD
{
	OutpFD* OD = nullptr; /*nullptr=dummy*/
	FrmlElem* Bool = nullptr;
	std::vector<AssignD*> Ass;
};
