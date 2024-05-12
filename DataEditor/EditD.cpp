#include "EditD.h"

EditD::EditD(uint8_t cols, uint8_t rows)
{
	FrstCol = 1;
	FrstRow = 2;
	LastCol = cols;
	LastRow = rows - 1;

	V.C1 = 1;
	V.R1 = 2;
	V.C2 = cols;
	V.R2 = rows - 1;
}

EditD::~EditD()
{
}

