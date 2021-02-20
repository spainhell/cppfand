#pragma once
#include "base.h"
#include "constants.h"

class FrmlElem;
class FileD;

class LocVar : public Chained // ø. 239
{
public:
	LocVar() = default;
	LocVar(std::string Name) { this->Name = Name; }
	bool IsPar = false; // urcuje, zda se jedna o vstupni parametr
	bool IsRetPar = false; // urcuje, zda jde o parametr predavany odkazem
	bool IsRetValue = false; // pridano navic - urcuje navratovou hodnotu funkce
	char FTyp = '\0';
	FileD* FD = nullptr;
	void* RecPtr = nullptr;
	std::string Name;
	char Op = '\0';
	WORD BPOfs = 0;
	FrmlElem* Init = nullptr;

	bool B = false;
	double R = 0.0;
	std::string S;
	WORD orig_S_length = 0;
};

class LocVarBlkD : public Chained // r228
{
public:
	LocVarBlkD() {  }
	//~LocVarBlkD() {
	//	for (size_t i = 0; i < vLocVar.size(); i ++) { delete vLocVar[i]; }
	//}
	LocVar* GetRoot();
	LocVar* FindByName(std::string Name);
	std::string FceName;
	std::vector<LocVar*> vLocVar;
	//LocVar* Root = nullptr;
	WORD NParam = 0;
	WORD Size = 0;
};
