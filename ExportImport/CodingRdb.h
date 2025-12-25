#pragma once

#include "../fandio/FieldDescr.h"
#include "../DataEditor/EditReader.h"


class DataEditor;

class CodingRdb
{
public:
	void CodeRdb(EditD* edit, bool Rotate);
	void CompressTxt(WORD IRec, LongStr* s, char Typ);
	void Wr(uint8_t c);
	void CodeF(FileD* file_d, Record* record, bool rotate, WORD IRec, FieldDescr* F, char Typ);
	void CompressCRdb(DataEditor* data_editor, EditD* edit);

private:
	WORD l = 0;
	WORD posUDLI = 0;
	LongStr* ss = nullptr;
};

