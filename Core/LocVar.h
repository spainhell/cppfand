#pragma once
#include "base.h"
#include "constants.h"


class FrmlElem;
class FileD;

class LocVar
{
public:
	LocVar() = default;
	LocVar(const std::string& name) { this->name = name; }
	std::string name;
	bool is_param = false;          // urcuje, zda se jedna o vstupni parametr
	bool is_return_param = false;   // urcuje, zda jde o parametr predavany odkazem
	bool is_return_value = false;   // pridano navic - urcuje navratovou hodnotu funkce
	char f_typ = '\0';
	FileD* FD = nullptr;
	uint8_t* record = nullptr;
	instr_type oper = _notdefined;
	FrmlElem* init = nullptr;

	bool B = false;
	double R = 0.0;
	std::string S;
};

class LocVarBlock
{
public:
	LocVarBlock() = default;
	LocVar* GetRoot();
	LocVar* FindByName(std::string Name);
	std::string func_name;
	std::vector<LocVar*> variables;
	size_t NParam = 0;
};
