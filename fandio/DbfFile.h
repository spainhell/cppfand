#pragma once

#include "../Core/FileD.h"

struct DBaseField
{
	std::string Name;
	char Typ = 0;
	int Displ = 0;
	BYTE Len = 0;
	BYTE Dec = 0;
	BYTE x2[14];
};

class DBaseHeader
{
public:
	DBaseHeader() = default;
	~DBaseHeader() {
		for (DBaseField* fld : flds) {
			delete fld;
		}
	}
	uint8_t Ver = 0;
	uint8_t Date[3]{ 0,0,0 };
	int32_t NRecs = 0;
	uint16_t HdLen = 0;
	uint16_t RecLen = 0;
	std::vector<DBaseField*> flds;
};

class DbfFile
{
public:
	static void WrDBaseHd(FileD* file_d);
	int MakeDbfDcl(pstring Nm);
};

