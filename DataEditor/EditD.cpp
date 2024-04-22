#include "EditD.h"

EditD::EditD(uint8_t cols, uint8_t rows)
{
	FrstCol = 1;
	FrstRow = 2;
	LastCol = cols;
	LastRow = rows - 1;
}

EditD::~EditD()
{
}

