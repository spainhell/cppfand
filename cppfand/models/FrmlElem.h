#pragma once
#include "../access.h"

class FrmlElem0 : public FrmlElem
{
public:
	FrmlElem0();
	FrmlElem0(size_t buff_size);
	FrmlElem* P1 = nullptr; FrmlElem* P2 = nullptr; FrmlElem* P3 = nullptr;
	FrmlElem* P4 = nullptr; FrmlElem* P5 = nullptr; FrmlElem* P6 = nullptr; // 0
	char Delim = '\0'; // 0
};

class FrmlElem1 : public FrmlElem
{
public:
	FrmlElem1();
	FrmlElem1(size_t buff_size);
	BYTE N01 = 0, N02 = 0, N03 = 0, N04 = 0;
	BYTE N11 = 0, N12 = 0, N13 = 0, N14 = 0;
	BYTE N21 = 0, N22 = 0, N23 = 0, N24 = 0;
	BYTE N31 = 0; // 1
	WORD W01 = 0, W02 = 0, W11 = 0, W12 = 0, W21 = 0, W22 = 0; // 1
};

class FrmlElem2 : public FrmlElem
{
public:
	FrmlElem2();
	FrmlElem2(size_t buff_size);
	double R = 0.0; // 2
};

class FrmlElem4 : public FrmlElem
{
public:
	FrmlElem4();
	FrmlElem4(size_t buff_size);
	pstring S; // 4
};

class FrmlElem5 : public FrmlElem
{
public:
	FrmlElem5();
	FrmlElem5(size_t buff_size);
	bool B = false; // 5
};

class FrmlElem6 : public FrmlElem
{
public:
	FrmlElem6();
	FrmlElem6(size_t buff_size);
	FrmlElem* PP1 = nullptr;
	pstring Mask; // 6
};

class FrmlElem7 : public FrmlElem
{
public:
	FrmlElem7();
	FrmlElem7(size_t buff_size);
	FieldDescr* Field = nullptr; // 7 {_field}
	FrmlElem* P011 = nullptr;
	FileD* File2 = nullptr;
	LinkD* LD = nullptr; // 7  {LD=nil for param} {_access} {LD=RecPtr} {_recvarfld}
};

class FrmlElem8 : public FrmlElem
{
public:
	FrmlElem8();
	FrmlElem8(size_t buff_size);
	FrmlElem* Frml = nullptr;
	FileD* NewFile = nullptr;
	void* NewRP = nullptr; // 8 {_newfile}
};

class FrmlElem9 : public FrmlElem
{
public:
	FrmlElem9();
	FrmlElem9(size_t buff_size);
	FileD* FD = nullptr; // 9 {_lastupdate, _generation}
};

class FrmlElem10 : public FrmlElem
{
public:
	FrmlElem10();
	FrmlElem10(size_t buff_size);
	WORD CatIRec = 0;
	FieldDescr* CatFld = nullptr; // 10 {_catfield}
};

class FrmlElem11 : public FrmlElem
{
public:
	FrmlElem11();
	FrmlElem11(size_t buff_size);
	FrmlElem* PPP1 = nullptr;
	FrmlElem* PP2 = nullptr;
	FieldDescr* FldD = nullptr; // 11 {_prompt}
};

class FrmlElem12 : public FrmlElem
{
public:
	FrmlElem12();
	FrmlElem12(size_t buff_size);
	FrmlElem* PPPP1 = nullptr; FrmlElem* PPP2 = nullptr;
	FrmlElem* PP3 = nullptr;
	pstring Options; // 12 {_pos,_replace}
};

class FrmlElem13 : public FrmlElem
{
public:
	FrmlElem13();
	FrmlElem13(size_t buff_size);
	FileD* FFD = nullptr;
	KeyD* Key = nullptr;
	FrmlElem* Arg[2]{ nullptr }; // 13 {_recno/typ='R' or 'S'/,_recnoabs,_recnolog}
};

class FrmlElem14 : public FrmlElem
{
public:
	FrmlElem14();
	FrmlElem14(size_t buff_size);
	FrmlElem* PPPPP1 = nullptr;
	FileD* RecFD = nullptr;
	FieldDescr* RecFldD = nullptr; // 14 {_accrecno,_isdeleted}
};

class FrmlElem15 : public FrmlElem
{
public:
	FrmlElem15();
	FrmlElem15(size_t buff_size);
	LinkD* LinkLD = nullptr; bool LinkFromRec = false;
	LocVar* LinkLV = nullptr; FrmlElem* LinkRecFrml = nullptr; // 15 {_link}
};

class FrmlElem16 : public FrmlElem
{
public:
	FrmlElem16();
	FrmlElem16(size_t buff_size);
	FrmlElem* PPPPPP1 = nullptr; FrmlElem* PPPP2 = nullptr;
	pstring* TxtPath = nullptr; WORD TxtCatIRec = 0; // 16 {_gettxt,_filesize}
};

class FrmlElem18 : public FrmlElem
{
public:
	FrmlElem18();
	FrmlElem18(size_t buff_size);
	WORD BPOfs = 0; // 18 { _getlocvar }
};

class FrmlElem19 : public FrmlElem
{
public:
	FrmlElem19();
	FrmlElem19(size_t buff_size);
	FuncD* FC = nullptr;
	FrmlListEl* FrmlL = nullptr; // 19 { _userfunc }
};

class FrmlElem20 : public FrmlElem
{
public:
	FrmlElem20();
	FrmlElem20(size_t buff_size);
	LocVar* LV = nullptr;
	KeyD* PackKey = nullptr; // 20 { _keyof,_lvdeleted }
};
class FrmlElem21 : public FrmlElem
{
public:
	FrmlElem21();
	FrmlElem21(size_t buff_size);
	FrmlElem* EvalP1 = nullptr;
	char EvalTyp = '\0';
	FileD* EvalFD = nullptr; // 21 {_eval}
};
class FrmlElem22 : public FrmlElem
{
public:
	FrmlElem22();
	FrmlElem22(size_t buff_size);
	XWKey* WKey = nullptr; // 22 {_indexnrecs}
};
class FrmlElem23 : public FrmlElem
{
public:
	FrmlElem23();
	FrmlElem23(size_t buff_size);
	FrmlElem* ownBool = nullptr;
	FrmlElem* ownSum = nullptr;
	LinkD* ownLD = nullptr; // 23 { _owned }
};