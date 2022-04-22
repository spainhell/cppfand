#pragma once

#include "Prolog.h"
#include "../cppfand/constants.h"
#include "../cppfand/FileD.h"

//extern WORD _Sg;

std::string SaveDb(TDatabase* Db, longint AA);
void RunProlog(RdbPos* Pos, std::string PredName);
