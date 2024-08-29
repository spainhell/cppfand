#pragma once
#include "../Core/Rdb.h"


void FandHelp(FileD* help_file, const std::string& name, bool InCWw);
void Help(RdbD* R, std::string name, bool InCWw);
void ClearHelpStkForCRdb();
