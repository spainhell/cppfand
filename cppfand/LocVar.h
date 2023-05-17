#pragma once
#include "base.h"
#include "constants.h"

class FrmlElem;
class FileD;

class LocVar : public Chained<LocVar>
{
public:
	LocVar() = default;
	LocVar(std::string Name) { this->Name = Name; }
	bool IsPar = false;      // urcuje, zda se jedna o vstupni parametr
	bool IsRetPar = false;   // urcuje, zda jde o parametr predavany odkazem
	bool IsRetValue = false; // pridano navic - urcuje navratovou hodnotu funkce
	char FTyp = '\0';
	FileD* FD = nullptr;
	void* RecPtr = nullptr;
	std::string Name;
	instr_type Op = _notdefined;
	FrmlElem* Init = nullptr;

	bool B = false;
	double R = 0.0;
	std::string S;
};

class LocVarBlkD : public Chained<LocVarBlkD>
{
public:
	LocVarBlkD() {  }
	LocVar* GetRoot();
	LocVar* FindByName(std::string Name);
	std::string FceName;
	std::vector<LocVar*> vLocVar;
	WORD NParam = 0;
};
