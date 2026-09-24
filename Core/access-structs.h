#pragma once
#include "base.h"
#include "Implicit.h"
#include "Project.h"
#include "../fandio/XKey.h"
#include "../fandio/KeyInD.h"
#include "../Common/RdbPos.h"


class Instr;
class FileD;
class LogicControl;
class FrmlElem;
class FrmlElemSum;
class FieldDescr;
class LocVarBlock;

struct LiRoots
{
	std::vector<LogicControl*> Chks;
	std::vector<Implicit*> Impls;
};

struct WRectFrml // r251
{
	FrmlElem* C1 = nullptr;
	FrmlElem* R1 = nullptr;
	FrmlElem* C2 = nullptr;
	FrmlElem* R2 = nullptr;
};

struct CompInpD // r402
{
	CompInpD* ChainBack = nullptr;
	char* InpArrPtr = nullptr;
	RdbPos InpRdbPos;
	size_t InpArrLen = 0;
	size_t CurrPos = 0;
	size_t OldErrPos = 0;
};