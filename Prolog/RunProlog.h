#pragma once

#include "Prolog.h"
#include "../cppfand/constants.h"
#include "../cppfand/FileD.h"

//extern WORD _Sg;

LongStr* SaveDb(TDatabase* Db, longint AA);
void RunProlog(RdbPos* Pos, std::string PredName);
