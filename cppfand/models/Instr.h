#pragma once
#include "../rdrun.h"
#include "../Graph.h"
#include "../EditOpt.h"

enum PInstrCode
{
	_menubox, _menubar, _ifthenelseP, _whiledo,
	_repeatuntil, _break, _exitP, _cancel, _save, _closefds,
	_window, _clrscr, _clrww, _clreol, _gotoxy, _display,
	_writeln, _comment, _setkeybuf, _clearkeybuf, _headline,
	_call, _exec, _copyfile, _proc, _lproc, _merge, _sort, _edit, _report,
	_edittxt, _printtxt, _puttxt, _sql,
	_asgnloc, _asgnpar, _asgnfield, _asgnedok, _asgnrand, _asgnusertoday,
	_randomize,
	_asgnusercode, _asgnusername,
	_asgnaccright, _asgnxnrecs,
	_asgnnrecs, _asgncatfield, _asgnrecfld, _asgnrecvar, _asgnclipbd,
	_turncat, _appendrec, _deleterec, _recallrec, _readrec, _writerec,
	_linkrec,
	_releasedrive, _mount, _indexfile, _getindex, _forall,
	_withshared, _withlocked, _withgraphics,
	_memdiag, _wait, _delay, _beepP, _sound, _nosound, _help, _setprinter,
	_graph, _putpixel, _line, _rectangle, _ellipse, _floodfill, _outtextxy,
	_backup, _backupm, _resetcat,
	_setedittxt, _setmouse, _checkfile, _login, _sqlrdwrtxt,
	_portout
};

class Instr
{
public:
	Instr(PInstrCode kind);
	PInstrCode Kind;
	Instr* Chain = nullptr;
};

class Instr_menu : public Instr
{
public:
	Instr_menu(PInstrCode Kind);
	~Instr_menu();
	FrmlElem* HdLine = nullptr;
	RdbD* HelpRdb = nullptr;
	bool WasESCBranch = false;
	Instr* ESCInstr = nullptr;
	std::vector<ChoiceD*> Choices;
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
	Instr* ElseInstr1 = nullptr;  // puvodne Instr a ElseInstr -> konflikt nazvu
	void AddInstr(Instr* i);
	void AddElseInstr(Instr* i);
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
	std::string ProcName;
	RdbPos PPos;
	BYTE N = 0;
	bool ExPar = false;
	std::vector<TypAndFrml> TArg;
	LocVarBlkD variables;
};

class Instr_lproc : public Instr
{
public:
	Instr_lproc();
	RdbPos lpPos;
	std::string lpName;
};

class Instr_call : public Instr
{
public:
	Instr_call();
	std::string RdbNm;
	std::string ProcNm;
	Instr_proc* ProcCall = nullptr;
};

class Instr_exec : public Instr
{
public:
	Instr_exec();
	std::string ProgPath;
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

enum class WriteType { write = 0, writeln = 1, message = 2, msgAndHelp = 3 };

class Instr_writeln : public Instr
{
public:
	Instr_writeln();
	WriteType LF = WriteType::write; /* 0-write, 1-writeln, 2-message, 3-message+help */;
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
	//LocVar* LV = nullptr;
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
	XKey* Key = nullptr;
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

class Instr_edit : public Instr
{
public:
	Instr_edit();
	FileD* EditFD = nullptr;
	EditOpt EO;
};

class Instr_report : public Instr
{
public:
	Instr_report();
	RprtOpt* RO = nullptr;
};

class Instr_edittxt : public Instr
{
public:
	Instr_edittxt(PInstrCode Kind);
	std::string TxtPath;
	WORD TxtCatIRec = 0;
	LocVar* TxtLV = nullptr;
	char EdTxtMode = '\0';
	std::vector<EdExitD*> ExD;
	BYTE WFlags = 0;
	FrmlElem* TxtPos = nullptr;
	FrmlElem* TxtXY = nullptr;
	FrmlElem* ErrMsg = nullptr;
	WRectFrml Ww;
	FrmlElem* Atr = nullptr;
	FrmlElem* Hd = nullptr;
	FrmlElem* Head = nullptr;
	FrmlElem* Last = nullptr;
	FrmlElem* CtrlLast = nullptr;
	FrmlElem* AltLast = nullptr;
	FrmlElem* ShiftLast = nullptr;
};

class Instr_puttxt : public Instr
{
public:
	Instr_puttxt();
	std::string TxtPath1; WORD TxtCatIRec1 = 0;
	FrmlElem* Txt = nullptr; bool App = false;
};

class Instr_releasedrive : public Instr
{
public:
	Instr_releasedrive();
	FrmlElem* Drive = nullptr;
};

class Instr_mount : public Instr
{
public:
	Instr_mount();
	WORD MountCatIRec = 0;
	bool MountNoCancel = false;
};

class Instr_indexfile : public Instr
{
public:
	Instr_indexfile();
	FileD* IndexFD = nullptr;
	bool Compress = false;
};

class Instr_getindex : public Instr
{
public:
	Instr_getindex();
	LocVar* giLV = nullptr; char giMode = '\0'; /*+,-,blank*/
	FrmlElem* giCond = nullptr; /* || RecNr-Frml */
	XKey* giKD = nullptr;
	KeyFldD* giKFlds = nullptr;
	KeyInD* giKIRoot = nullptr;
	bool giSQLFilter = false;
	char giOwnerTyp = '\0';
	LinkD* giLD = nullptr;
	LocVar* giLV2 = nullptr;
};

class Instr_window : public Instr
{
public:
	Instr_window();
	WRectFrml W;
	FrmlElem* Attr = nullptr;
	Instr* WwInstr = nullptr;
	FrmlElem* Top = nullptr;
	BYTE WithWFlags = 0;
};

class Instr_clrww : public Instr
{
public:
	Instr_clrww();
	WRectFrml W2;
	FrmlElem* Attr2 = nullptr;
	FrmlElem* FillC = nullptr;
};

class Instr_forall : public Instr
{
public:
	Instr_forall();
	FileD* CFD = nullptr;
	XKey* CKey = nullptr;
	LocVar* CVar = nullptr;
	LocVar* CRecVar = nullptr;
	KeyInD* CKIRoot = nullptr;
	FrmlElem* CBool = nullptr; /*or SQLTxt*/
	Instr* CInstr = nullptr;
	LinkD* CLD = nullptr;
	bool CWIdx = false, inSQL = false, CSQLFilter = false, CProcent = false;
	char COwnerTyp = '\0';
	LocVar* CLV = nullptr;
};

class Instr_withshared : public Instr
{
public:
	Instr_withshared(PInstrCode Kind);
	Instr* WDoInstr = nullptr;
	Instr* WElseInstr = nullptr;
	bool WasElse = false;
	LockD WLD;
};

class Instr_graph : public Instr
{
public:
	Instr_graph();
	GraphD* GD = nullptr;
};

class Instr_putpixel : public Instr
{
public:
	Instr_putpixel(PInstrCode Kind);
	FrmlElem* Par1 = nullptr;
	FrmlElem* Par2 = nullptr;
	FrmlElem* Par3 = nullptr;
	FrmlElem* Par4 = nullptr;
	FrmlElem* Par5 = nullptr;
	FrmlElem* Par6 = nullptr;
	FrmlElem* Par7 = nullptr;
	FrmlElem* Par8 = nullptr;
	FrmlElem* Par9 = nullptr;
	FrmlElem* Par10 = nullptr;
	FrmlElem* Par11 = nullptr;
};

class Instr_backup : public Instr
{
public:
	Instr_backup(PInstrCode Kind);
	WORD BrCatIRec = 0;
	bool IsBackup = false, NoCompress = false, BrNoCancel = false;
	BYTE bmX[5]{ 0 };
	FrmlElem* bmDir = nullptr;
	FrmlElem* bmMasks = nullptr; /*backup only*/
	bool bmSubDir = false, bmOverwr = false;
};

class Instr_closefds : public Instr
{
public:
	Instr_closefds();
	FileD* clFD = nullptr;
};

class Instr_setedittxt : public Instr
{
public:
	Instr_setedittxt();
	FrmlElem* Insert = nullptr;
	FrmlElem* Indent = nullptr;
	FrmlElem* Wrap = nullptr;
	FrmlElem* Just = nullptr;
	FrmlElem* ColBlk = nullptr;
	FrmlElem* Left = nullptr;
	FrmlElem* Right = nullptr;
};

class Instr_setmouse : public Instr
{
public:
	Instr_setmouse();
	FrmlElem* MouseX = nullptr; FrmlElem* MouseY = nullptr; FrmlElem* Show = nullptr;
};

class Instr_checkfile : public Instr
{
public:
	Instr_checkfile();
	FileD* cfFD = nullptr;
	std::string cfPath;
	WORD cfCatIRec = 0;
};

class Instr_login : public Instr
{
public:
	Instr_login();
	FrmlElem* liName = nullptr;
	FrmlElem* liPassWord = nullptr;
};

class Instr_sqlrdwrtxt : public Instr
{
public:
	Instr_sqlrdwrtxt();
	pstring* TxtPath2 = nullptr;
	WORD TxtCatIRec2 = 0;
	bool IsRead = false;
	FileD* sqlFD = nullptr; XKey* sqlKey = nullptr;
	FieldDescr* sqlFldD = nullptr; FrmlElem* sqlXStr = nullptr;
};

class Instr_portout : public Instr
{
public:
	Instr_portout();
	FrmlElem* IsWord = nullptr;
	FrmlElem* Port = nullptr;
	FrmlElem* PortWhat = nullptr;
};
