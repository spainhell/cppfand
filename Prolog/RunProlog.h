#pragma once

#include "Prolog.h"
#include "../CppFand/constants.h"
#include "../CppFand/FileD.h"

//extern WORD _Sg;

std::string SaveDb(TDatabase* Db, int AA);
void RunProlog(RdbPos* Pos, std::string PredName);
