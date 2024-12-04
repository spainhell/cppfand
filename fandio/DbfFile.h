#pragma once

#include "../Core/FileD.h"


class DbfFile
{
public:
	static void WrDBaseHd(FileD* file_d);
	int MakeDbfDcl(pstring Nm);
};

