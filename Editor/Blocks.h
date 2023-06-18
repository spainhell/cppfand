#pragma once
#include "../Core/constants.h"

class Blocks
{
public:
	Blocks();
	size_t LineAbs(int Ln);
	bool LineInBlock(int Ln);
	bool LineBndBlock(int Ln);
	WORD BegBLn = 0;
	WORD EndBLn = 0;
	WORD BegBPos = 0;
	WORD EndBPos = 0;

private:

};

