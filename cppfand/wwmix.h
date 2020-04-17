#pragma once
#include "constants.h"
#include "pstring.h"

class wwmix
{
public:
	static struct SS
	{
		pstring* Pointto;
		bool Abcd, AscDesc, Subset, ImplAll, Empty;
		WORD Size;
		char Tag;
	} ss;
};

