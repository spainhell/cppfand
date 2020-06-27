#pragma once
#include "../access.h"

class FrmlElem0 : public FrmlElem
{
public:
	FrmlElem0(BYTE Op, size_t buff_size);
	FrmlElem* P1 = nullptr; FrmlElem* P2 = nullptr; FrmlElem* P3 = nullptr;
	FrmlElem* P4 = nullptr; FrmlElem* P5 = nullptr; FrmlElem* P6 = nullptr; // 0
	char Delim = '\0'; // 0
	BYTE N11 = 0, N12 = 0;
	BYTE N21 = 0, N22 = 0;
	BYTE N31 = 0;
	WORD W11 = 0;
	BYTE buff[64]{ 0 };
	LocVar* LV1 = nullptr; // pro potreby smycky FOR
};

class FrmlElem1 : public FrmlElem
{
public:
	FrmlElem1(BYTE Op, size_t buff_size);
	BYTE N01 = 0, N02 = 0, N03 = 0, N04 = 0;
	BYTE N11 = 0, N12 = 0, N13 = 0, N14 = 0;
	BYTE N21 = 0, N22 = 0, N23 = 0, N24 = 0;
	BYTE N31 = 0; // 1
	WORD W01 = 0, W02 = 0, W11 = 0, W12 = 0, W21 = 0, W22 = 0; // 1
};

class FrmlElem2 : public FrmlElem
{
public:
	FrmlElem2(BYTE Op, size_t buff_size);
	FrmlElem2(BYTE Op, size_t buff_size, double value);
	double R = 0.0; // 2
};

class FrmlElem4 : public FrmlElem
{
public:
	FrmlElem4(BYTE Op, size_t buff_size);
	FrmlElem4(BYTE Op, size_t buff_size, pstring value);
	FrmlElem4(BYTE Op, size_t buff_size, pstring* value);
	pstring S; // 4
};

class FrmlElem5 : public FrmlElem
{
public:
	FrmlElem5(BYTE Op, size_t buff_size);
	FrmlElem5(BYTE Op, size_t buff_size, bool value);
	bool B = false; // 5
};

class FrmlElem6 : public FrmlElem
{
public:
	FrmlElem6(BYTE Op, size_t buff_size);
	FrmlElem* PP1 = nullptr;
	pstring Mask; // 6
};

class FrmlElem7 : public FrmlElem
{
public:
	FrmlElem7(BYTE Op, size_t buff_size);
	FieldDescr* Field = nullptr; // 7 {_field}
	FrmlElem* P011 = nullptr;
	FileD* File2 = nullptr;
	LinkD* LD = nullptr; // 7  {LD=nil for param} {_access} {LD=RecPtr} {_recvarfld}
};

class FrmlElem8 : public FrmlElem
{
public:
	FrmlElem8(BYTE Op, size_t buff_size);
	FrmlElem* Frml = nullptr;
	FileD* NewFile = nullptr;
	void* NewRP = nullptr; // 8 {_newfile}
};

class FrmlElem9 : public FrmlElem
{
public:
	FrmlElem9(BYTE Op, size_t buff_size);
	FileD* FD = nullptr; // 9 {_lastupdate, _generation}
};

class FrmlElem10 : public FrmlElem
{
public:
	FrmlElem10(BYTE Op, size_t buff_size);
	WORD CatIRec = 0;
	FieldDescr* CatFld = nullptr; // 10 {_catfield}
};

class FrmlElem11 : public FrmlElem
{
public:
	FrmlElem11(BYTE Op, size_t buff_size);
	FrmlElem* PPP1 = nullptr;
	FrmlElem* PP2 = nullptr;
	FieldDescr* FldD = nullptr; // 11 {_prompt}
};

class FrmlElem12 : public FrmlElem
{
public:
	FrmlElem12(BYTE Op, size_t buff_size);
	FrmlElem* PPPP1 = nullptr; FrmlElem* PPP2 = nullptr;
	FrmlElem* PP3 = nullptr;
	pstring Options; // 12 {_pos,_replace}
};

class FrmlElem13 : public FrmlElem
{
public:
	FrmlElem13(BYTE Op, size_t buff_size);
	FileD* FFD = nullptr;
	KeyD* Key = nullptr;
	void SaveArgs(FrmlElem* arguments[], size_t count);
	std::vector<FrmlElem*> Arg;
	//FrmlElem* Arg[2]{ nullptr }; // 13 {_recno/typ='R' or 'S'/,_recnoabs,_recnolog}
};

class FrmlElem14 : public FrmlElem
{
public:
	FrmlElem14(BYTE Op, size_t buff_size);
	FrmlElem* PPPPP1 = nullptr;
	FileD* RecFD = nullptr;
	FieldDescr* RecFldD = nullptr; // 14 {_accrecno,_isdeleted}
};

class FrmlElem15 : public FrmlElem
{
public:
	FrmlElem15(BYTE Op, size_t buff_size);
	LinkD* LinkLD = nullptr; bool LinkFromRec = false;
	LocVar* LinkLV = nullptr; FrmlElem* LinkRecFrml = nullptr; // 15 {_link}
};

class FrmlElem16 : public FrmlElem
{
public:
	FrmlElem16(BYTE Op, size_t buff_size);
	FrmlElem* PPPPPP1 = nullptr; FrmlElem* PPPP2 = nullptr;
	pstring* TxtPath = nullptr; WORD TxtCatIRec = 0; // 16 {_gettxt,_filesize}
};

class FrmlElem18 : public FrmlElem
{
public:
	FrmlElem18(BYTE Op, size_t buff_size);
	FrmlElem18(BYTE Op, LocVar* lv);
	LocVar* locvar = nullptr;
	//WORD BPOfs = 0; // 18 { _getlocvar }
	BYTE buff[64]{ 0 };
};

class FrmlElem19 : public FrmlElem
{
public:
	FrmlElem19(BYTE Op, size_t buff_size);
	FuncD* FC = nullptr;
	FrmlListEl* FrmlL = nullptr; // 19 { _userfunc }
};

class FrmlElem20 : public FrmlElem
{
public:
	FrmlElem20(BYTE Op, size_t buff_size);
	LocVar* LV = nullptr;
	KeyD* PackKey = nullptr; // 20 { _keyof,_lvdeleted }
};
class FrmlElem21 : public FrmlElem
{
public:
	FrmlElem21(BYTE Op, size_t buff_size);
	FrmlElem* EvalP1 = nullptr;
	char EvalTyp = '\0';
	FileD* EvalFD = nullptr; // 21 {_eval}
};
class FrmlElem22 : public FrmlElem
{
public:
	FrmlElem22(BYTE Op, size_t buff_size);
	XWKey* WKey = nullptr; // 22 {_indexnrecs}
};
class FrmlElem23 : public FrmlElem
{
public:
	FrmlElem23(BYTE Op, size_t buff_size);
	FrmlElem* ownBool = nullptr;
	FrmlElem* ownSum = nullptr;
	LinkD* ownLD = nullptr; // 23 { _owned }
};

// pro zjisteni polozky v mnozine IN[] (string nebo double)
class FrmlElemIn : public FrmlElem
{
public:
	FrmlElemIn(BYTE Op);
	FrmlElem* P1 = nullptr;
	BYTE param1 = 0;
	BYTE param2 = 0;
	std::vector<std::string> strings;
	std::vector<double> reals;
	std::vector<std::pair<std::string, std::string>> strings_range;
	std::vector<std::pair<double, double>> reals_range;
};
