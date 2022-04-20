#pragma once

#include "../cppfand/constants.h"
#include "../cppfand/FileD.h"

//extern WORD _Sg;

LongStr* SaveDb(WORD DbOfs/*PDatabase*/, longint AA);
void RunProlog(RdbPos* Pos, std::string* PredName);
