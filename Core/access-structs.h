#pragma once
#include "base.h"
#include "Implicit.h"
#include "Rdb.h"
#include "../fandio/XKey.h"


class Instr;
class FileD;
class LogicControl;
class FrmlElem;
class FrmlElemSum;
class FieldDescr;
class LocVarBlkD;

struct KeyInD
{
	std::vector<FrmlElem*> FL1;
	std::vector<FrmlElem*> FL2;
	int XNrBeg = 0, N = 0;
	std::string X1;
	std::string X2;
};

struct structXPath
{
	int Page = 0;
	WORD I = 0;
};

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