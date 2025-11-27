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
	//delete OldRec;
	//OldRec = nullptr;
	//delete NewRec;
	//NewRec = nullptr;
	delete DownRecord; DownRecord = nullptr;
}

std::vector<EFldD*>::iterator EditD::GetEFldIter(EFldD* e_fld)
{
	// find e_fld in FirstFld and return it as iterator
	for (std::vector<EFldD*>::iterator it = FirstFld.begin(); it != FirstFld.end(); ++it) {
		if (*it == e_fld) {
			return it;
		}
	}
	return FirstFld.end();
}

