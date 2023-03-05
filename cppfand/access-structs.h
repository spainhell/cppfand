#pragma once
#include "base.h"
#include "Chained.h"
#include "Rdb.h"
#include "../Indexes/XKey.h"


class Instr;
class FileD;
class ChkD;
class FrmlElem;
class FrmlElemSum;
class FieldDescr;
class LocVarBlkD;

struct FieldListEl : public Chained<FieldListEl>
{
	FieldDescr* FldD = nullptr;
};
typedef FieldListEl* FieldList;

struct FrmlListEl : public Chained<FrmlListEl>
{
	FrmlElem* Frml = nullptr;
};
typedef FrmlListEl* FrmlList;

struct StringListEl : public Chained<StringListEl>
{
	std::string S;
};
typedef StringListEl* StringList;

struct KeyListEl : public Chained<KeyListEl>
{
	XKey* Key = nullptr;
};

struct KeyInD : public Chained<KeyListEl>
{
	FrmlListEl* FL1 = nullptr;
	FrmlListEl* FL2 = nullptr;
	longint XNrBeg = 0, N = 0;
	std::string X1;
	std::string X2;
};

struct structXPath
{
	longint Page = 0;
	WORD I = 0;
};

struct ImplD : public Chained<ImplD>
{
	FieldDescr* FldD = nullptr;
	FrmlElem* Frml = nullptr;
};
typedef ImplD* ImplDPtr;

struct LiRoots
{
	ChkD* Chks = nullptr;
	ImplD* Impls = nullptr;
};
typedef LiRoots* LiRootsPtr;

struct DBaseFld
{
	pstring Name;
	char Typ = 0;
	longint Displ = 0;
	BYTE Len = 0, Dec = 0;
	BYTE x2[14];
};

struct DBaseHd
{
	BYTE Ver = 0;
	BYTE Date[4]{ 0,0,0,0 };
	longint NRecs = 0;
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