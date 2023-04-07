#pragma once

#include "Prolog.h"
#include "../cppfand/constants.h"
#include "../cppfand/FileD.h"

//extern WORD _Sg;

std::string SaveDb(TDatabase* Db, int AA);
void RunProlog(RdbPos* Pos, std::string PredName);
