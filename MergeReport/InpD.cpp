#include "InpD.h"
#include "../Common/Record.h"

InpD::InpD()
{
}

InpD::~InpD()
{
	delete RecPtr;
	delete ForwRecPtr;
}