#pragma once
#include "../rdrun.h"

//class Chained;
//class FrmlElem;
//struct RdbD;
//struct ChoiceD;
//struct RdbPos;


class Instr_menu : public Instr 
{
public:
	Instr_menu(PInstrCode Kind);
	FrmlElem* HdLine = nullptr;
	RdbD* HelpRdb = nullptr;
	bool WasESCBranch = false;
	Instr* ESCInstr = nullptr;
	ChoiceD* Choices = nullptr;
	bool Loop = false, PullDown = false, Shdw = false;
	FrmlElem* X = nullptr;
	FrmlElem* Y = nullptr;
	FrmlElem* XSz = nullptr;
	FrmlElem* mAttr[4]{ nullptr };
};

class Instr_loops : public Instr
{
public:
	Instr_loops(PInstrCode Kind);
	FrmlElem* Bool = nullptr;
	Instr* Instr1 = nullptr;
	Instr* ElseInstr1 = nullptr;  // pùvodnì Instr a ElseInstr -> konflikt názvù
};

class Instr_merge_display : public Instr
{
public:
	Instr_merge_display(PInstrCode Kind);
	RdbPos Pos;
};

class Instr_proc : public Instr
{
public:
	Instr_proc(size_t TArg_Count);
	RdbPos PPos;
	BYTE N = 0;
	bool ExPar = false;
	std::vector<TypAndFrml> TArg;
};

class Instr_lproc : public Instr
{
public:
	Instr_lproc();
	RdbPos lpPos;
	pstring* lpName = nullptr;
};

class Instr_call : public Instr
{
public:
	Instr_call();
	pstring* RdbNm = nullptr;
	pstring* ProcNm = nullptr;
	Instr_proc* ProcCall = nullptr;
};

class Instr_exec : public Instr
{
public:
	Instr_exec();
	pstring* ProgPath = nullptr;
	WORD ProgCatIRec = 0;
	bool NoCancel = false, FreeMm = false, LdFont = false, TextMd = false;
	FrmlElem* Param = nullptr;
};

class Instr_copyfile : public Instr
{
public:
	Instr_copyfile();
	CopyD* CD = nullptr;
};

class Instr_writeln : public Instr
{
public:
	Instr_writeln();
	BYTE LF = 0; /*0-write,1-writeln,2-message,3-message+help*/;
	WrLnD WD;
	RdbD* mHlpRdb = nullptr;
	FrmlElem* mHlpFrml = nullptr;
};

class Instr_gotoxy : public Instr
{
public:
	Instr_gotoxy();
	FrmlElem* GoX = nullptr;
	FrmlElem* GoY = nullptr;
};

class Instr_assign : public Instr
{
public:
	Instr_assign(PInstrCode Kind);
	FrmlElem* Frml = nullptr;
	bool Add = false;
	LocVar* AssLV = nullptr;
	FrmlElem* Frml1 = nullptr;
	bool Add1 = false;
	FileD* FD = nullptr;
	FieldDescr* FldD = nullptr;
	FrmlElem* RecFrml = nullptr;
	bool Indexarg = false;

	FrmlElem* Frml2 = nullptr; bool Add2 = false;
	LocVar* AssLV2 = nullptr; FieldDescr* RecFldD = nullptr;

	FrmlElem* Frml3 = nullptr; FileD* FD3 = nullptr;
	WORD CatIRec = 0; FieldDescr* CatFld = nullptr;

	LocVar* RecLV1 = nullptr; LocVar* RecLV2 = nullptr;
	AssignD* Ass = nullptr;
	LinkD* LinkLD = nullptr;
	XWKey* xnrIdx = nullptr;

	// pridano navic, chybi u volani LVAssignFrml(iPD->LV, MyBP, iPD->Add, iPD->Frml);
	// v runproc.cpp
	LocVar* LV = nullptr;
};

class Instr_help : public Instr
{
public:
	Instr_help();
	FrmlElem* Frml0 = nullptr;
	RdbD* HelpRdb0 = nullptr;
};

class Instr_recs : public Instr
{
public:
	Instr_recs(PInstrCode Kind);
	FrmlElem* RecNr = nullptr;
	bool AdUpd = false;
	LocVar* LV = nullptr;
	bool ByKey = false;
	KeyD* Key = nullptr;
	char CompOp = '\0';
	FileD* RecFD = nullptr;
};

class Instr_turncat : public Instr
{
public:
	Instr_turncat();
	FileD* NextGenFD = nullptr;
	WORD FrstCatIRec = 0, NCatIRecs = 0;
	FrmlElem* TCFrml = nullptr;
};

class Instr_sort : public Instr
{
public:
	Instr_sort();
	~Instr_sort();
	FileD* SortFD = nullptr;
	KeyFldD* SK = nullptr;
};