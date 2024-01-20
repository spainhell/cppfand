#include "Blocks.h"

Blocks::Blocks()
{
}

size_t Blocks::LineAbs(int Ln)
{
	size_t result = Ln; // Part.LineP + Ln;
	if (Ln < 1) {
		result = 1;
	}
	return result;
}

bool Blocks::LineInBlock(int Ln)
{
	if ((LineAbs(Ln) > BegBLn) && (LineAbs(Ln) < EndBLn)) {
		return true;
	}
	else {
		return false;
	}
}

bool Blocks::LineBndBlock(int Ln)
{
	if ((LineAbs(Ln) == BegBLn) || (LineAbs(Ln) == EndBLn)) {
		return true;
	}
	else {
		return false;
	}
}