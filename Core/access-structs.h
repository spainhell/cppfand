#pragma once
#include "base.h"
#include "Rdb.h"
#include "../fandio/XKey.h"


class Instr;
class FileD;
class ChkD;
class FrmlElem;
class FrmlElemSum;
class FieldDescr;
class LocVarBlkD;

//struct FieldListEl : public Chained<FieldListEl>
//{
//	FieldDescr* FldD = nullptr;
//};
//typedef FieldListEl* FieldList;
// std::vector<FieldDescr*>&

//struct FrmlListEl : public Chained<FrmlListEl>
//{
//	FrmlElem* Frml = nullptr;
//};
//typedef FrmlListEl* FrmlList;
// std::vector<FrmlElem*>&

//struct StringListEl : public Chained<StringListEl>
//{
//	std::string S;
//};
//typedef StringListEl* StringList;

//struct KeyListEl : public Chained<KeyListEl>
//{
//	XKey* Key = nullptr;
//};
// std::vector<XKey*>

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

struct ImplD
{
	FieldDescr* FldD = nullptr;
	FrmlElem* Frml = nullptr;
};

struct LiRoots
{
	std::vector<ChkD*> Chks;
	std::vector<ImplD*> Impls;
};

struct DBaseFld
{
	pstring Name;
	char Typ = 0;
	int Displ = 0;
	BYTE Len = 0, Dec = 0;
	BYTE x2[14];
};

struct DBaseHd
{
	BYTE Ver = 0;
	BYTE Date[4]{ 0,0,0,0 };
	int NRecs = 0;
	WORD HdLen = 0, RecLen = 0;
	DBaseFld Flds[1];
};

class LinkD
{
public:
	LinkD() = default;
	LinkD(const LinkD& orig)
	{
		this->IndexRoot = orig.IndexRoot;
		this->MemberRef = orig.MemberRef;
		this->Args = orig.Args;
		this->FromFD = orig.FromFD;
		this->ToFD = orig.ToFD;
		this->ToKey = orig.ToKey;
		this->RoleName = orig.RoleName;
	}
	WORD IndexRoot = 0; // 0 - non index file || to only primary key
	BYTE MemberRef = 0; // { 0-no, 1-!, 2-!!(no delete)}
	std::vector<KeyFldD*> Args;
	FileD* FromFD = nullptr;
	FileD* ToFD = nullptr;
	XKey* ToKey = nullptr;
	std::string RoleName;
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