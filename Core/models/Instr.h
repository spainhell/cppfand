#pragma once
#include "../rdrun.h"
#include "../Graph.h"
#include "../EditOpt.h"
#include "../../MergeReport/RprtOpt.h"
#include "../../TextEditor/TextEditor.h"

enum class PInstrCode
{
	_appendRec, _asgnAccRight, _asgnCatField, _asgnClipbd, _asgnEdOk, _asgnField, _asgnloc, _asgnnrecs,
	_asgnpar, _asgnrand, _asgnrecfld, _asgnrecvar, _asgnusercode, _asgnusername, _asgnusertoday, _asgnxnrecs,
	_backup, _backupm, _beepP, _break, _call, _cancel, _clearkeybuf, _closefds, _clreol, _clrscr, _clrww,
	_comment, _copyfile, _delay, _deleterec, _display, _edit, _edittxt, _ellipse, _exec, _exitP,
	_floodfill, _forall, _getindex, _gotoxy, _graph, _headline, _help, _checkfile, _ifthenelseP, _indexfile,
	_line, _linkrec, _login, _lproc, _memdiag, _menubar, _menubox, _merge, _mount, _nosound, _outtextxy,
	_portout, _printtxt, _proc, _putpixel, _puttxt, _randomize, _readrec, _recallrec, _rectangle, _releasedrive,
	_repeatuntil, _report, _resetcat, _save, _setedittxt, _setkeybuf, _setmouse, _setprinter, _sort, _sound,
	_sql, _sqlrdwrtxt, _turncat, _wait, _whiledo, _window, _withgraphics, _withlocked, _withshared,
	_writeln, _writerec /*, _not_defined*/
};

class Instr
{
public:
	Instr(PInstrCode kind);
	PInstrCode Kind;
	virtual ~Instr();
};

class Instr_menu : public Instr
{
public:
	Instr_menu(PInstrCode Kind);
	~Instr_menu() override;
	FrmlElem* HdLine = nullptr;
	RdbD* HelpRdb = nullptr;
	bool WasESCBranch = false;
	std::vector<Instr*> ESCInstr;
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
	~Instr_loops() override;
	FrmlElem* Bool = nullptr;
	std::vector<Instr*> v_instr;
	std::vector<Instr*> v_else_instr;
};

class Instr_merge_display : public Instr
{
public:
	Instr_merge_display(PInstrCode Kind);
	~Instr_merge_display() override;
	RdbPos Pos;
};

class Instr_proc : public Instr
{
public:
	Instr_proc(size_t TArg_Count);
	~Instr_proc() override;
	std::string ProcName;
	RdbPos PPos;
	uint8_t N = 0;
	bool ExPar = false;
	std::vector<TypAndFrml> TArg;
	LocVarBlock loc_var_block;
};

class Instr_lproc : public Instr
{
public:
	Instr_lproc();
	~Instr_lproc() override;
	RdbPos lpPos;
	std::string name;	// chapter name
	std::string lpName; // predicate name - default is 'main'
};

class Instr_call : public Instr
{
public:
	Instr_call();
	~Instr_call() override;
	std::string RdbNm;
	std::string ProcNm;
	Instr_proc* ProcCall = nullptr;
};

class Instr_exec : public Instr
{
public:
	Instr_exec();
	~Instr_exec() override;
	std::string ProgPath;
	WORD ProgCatIRec = 0;
	bool NoCancel = false, FreeMm = false, LdFont = false, TextMd = false;
	FrmlElem* Param = nullptr;
};

class Instr_copyfile : public Instr
{
public:
	Instr_copyfile();
	~Instr_copyfile() override;
	CopyD* CD = nullptr;
};

enum class WriteType { write = 0, writeln = 1, message = 2, msgAndHelp = 3 };

class Instr_writeln : public Instr
{
public:
	Instr_writeln();
	~Instr_writeln() override;
	WriteType LF = WriteType::write; /* 0-write, 1-writeln, 2-message, 3-message+help */
	std::vector<WrLnD*> WD;
	RdbD* mHlpRdb = nullptr;
	FrmlElem* mHlpFrml = nullptr;
};

class Instr_gotoxy : public Instr
{
public:
	Instr_gotoxy();
	~Instr_gotoxy() override;
	FrmlElem* GoX = nullptr;
	FrmlElem* GoY = nullptr;
};

class Instr_assign : public Instr
{
public:
	Instr_assign(PInstrCode Kind);
	~Instr_assign() override;
	FrmlElem* Frml = nullptr;
	bool Add = false;
	LocVar* AssLV = nullptr;
	FrmlElem* Frml1 = nullptr;
	bool Add1 = false;
	FileD* FD = nullptr;
	FieldDescr* FldD = nullptr;
	FrmlElem* RecFrml = nullptr;
	bool Indexarg = false;

	FrmlElem* Frml2 = nullptr;
	bool Add2 = false;
	LocVar* AssLV2 = nullptr;
	FieldDescr* RecFldD = nullptr;

	FrmlElem* Frml3 = nullptr;
	FileD* FD3 = nullptr;
	WORD CatIRec = 0;
	FieldDescr* CatFld = nullptr;

	LocVar* RecLV1 = nullptr;
	LocVar* RecLV2 = nullptr;
	std::vector<AssignD*> Ass;
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
	~Instr_help() override;
	FrmlElem* Frml0 = nullptr;
	RdbD* HelpRdb0 = nullptr;
};

class Instr_recs : public Instr
{
public:
	Instr_recs(PInstrCode Kind);
	~Instr_recs() override;
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
	~Instr_turncat() override;
	FileD* NextGenFD = nullptr;
	int FrstCatIRec = 0;
	int NCatIRecs = 0;
	FrmlElem* TCFrml = nullptr;
};

class Instr_sort : public Instr
{
public:
	Instr_sort();
	~Instr_sort() override;
	FileD* SortFD = nullptr;
	std::vector<KeyFldD*> SK;
};

class Instr_edit : public Instr
{
public:
	Instr_edit();
	~Instr_edit() override;
	FileD* EditFD = nullptr;
	EditOpt options;
};

class Instr_report : public Instr
{
public:
	Instr_report();
	~Instr_report() override;
	RprtOpt* RO = nullptr;
};

class Instr_edittxt : public Instr
{
public:
	Instr_edittxt(PInstrCode Kind);
	~Instr_edittxt() override;
	std::string TxtPath;
	WORD TxtCatIRec = 0;
	LocVar* TxtLV = nullptr;
	EditorMode EdTxtMode = EditorMode::Unknown;
	std::vector<EdExitD*> ExD;
	uint8_t WFlags = 0;
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
	~Instr_puttxt() override;
	std::string TxtPath1;
	WORD TxtCatIRec1 = 0;
	FrmlElem* Txt = nullptr;
	bool App = false;
};

class Instr_releasedrive : public Instr
{
public:
	Instr_releasedrive();
	~Instr_releasedrive() override;
	FrmlElem* Drive = nullptr;
};

class Instr_mount : public Instr
{
public:
	Instr_mount();
	~Instr_mount() override;
	int MountCatIRec = 0;
	bool MountNoCancel = false;
};

class Instr_indexfile : public Instr
{
public:
	Instr_indexfile();
	~Instr_indexfile() override;
	FileD* IndexFD = nullptr;
	bool Compress = false;
};

class Instr_getindex : public Instr
{
public:
	Instr_getindex();
	~Instr_getindex() override;
	LocVar* loc_var1 = nullptr;
	char mode = '\0'; /*+,-,blank*/
	FrmlElem* condition = nullptr; /* || RecNr-Frml */
	XKey* keys = nullptr;
	std::vector<KeyFldD*> key_fields;
	std::vector<KeyInD*> key_in_root;
	bool sql_filter = false;
	char owner_type = '\0';
	LinkD* link = nullptr;
	LocVar* loc_var2 = nullptr;
};

class Instr_window : public Instr
{
public:
	Instr_window();
	~Instr_window() override;
	WRectFrml W;
	FrmlElem* Attr = nullptr;
	std::vector<Instr*> v_ww_instr;
	FrmlElem* Top = nullptr;
	uint8_t WithWFlags = 0;
};

class Instr_clrww : public Instr
{
public:
	Instr_clrww();
	~Instr_clrww() override;
	WRectFrml W2;
	FrmlElem* Attr2 = nullptr;
	FrmlElem* FillC = nullptr;
};

class Instr_forall : public Instr
{
public:
	Instr_forall();
	~Instr_forall() override;
	FileD* CFD = nullptr;
	XKey* CKey = nullptr;
	LocVar* CVar = nullptr;
	LocVar* CRecVar = nullptr;
	std::vector<KeyInD*> CKIRoot;
	FrmlElem* CBool = nullptr; /*or SQLTxt*/
	std::vector<Instr*> CInstr;
	LinkD* CLD = nullptr;
	bool CWIdx = false, inSQL = false, CSQLFilter = false, CProcent = false;
	char COwnerTyp = '\0';
	LocVar* CLV = nullptr;
};

class Instr_withshared : public Instr
{
public:
	Instr_withshared(PInstrCode Kind);
	~Instr_withshared() override;
	std::vector<Instr*> WDoInstr;
	std::vector<Instr*> WElseInstr;
	bool WasElse = false;
	std::vector<LockD*> WLD;
};

class Instr_graph : public Instr
{
public:
	Instr_graph();
	~Instr_graph() override;
	GraphD* GD = nullptr;
};

class Instr_putpixel : public Instr
{
public:
	Instr_putpixel(PInstrCode Kind);
	~Instr_putpixel() override;
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
	~Instr_backup() override;
	int BrCatIRec = 0;
	bool IsBackup = false, NoCompress = false, BrNoCancel = false;
	uint8_t bmX[5]{ 0 };
	FrmlElem* bmDir = nullptr;
	FrmlElem* bmMasks = nullptr; /*backup only*/
	bool bmSubDir = false, bmOverwr = false;
};

class Instr_closefds : public Instr
{
public:
	Instr_closefds();
	~Instr_closefds() override;
	FileD* clFD = nullptr;
};

class Instr_setedittxt : public Instr
{
public:
	Instr_setedittxt();
	~Instr_setedittxt() override;
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
	~Instr_setmouse() override;
	FrmlElem* MouseX = nullptr;
	FrmlElem* MouseY = nullptr;
	FrmlElem* Show = nullptr;
};

class Instr_checkfile : public Instr
{
public:
	Instr_checkfile();
	~Instr_checkfile() override;
	FileD* cfFD = nullptr;
	std::string cfPath;
	WORD cfCatIRec = 0;
};

class Instr_login : public Instr
{
public:
	Instr_login();
	~Instr_login() override;
	FrmlElem* liName = nullptr;
	FrmlElem* liPassWord = nullptr;
};

class Instr_sqlrdwrtxt : public Instr
{
public:
	Instr_sqlrdwrtxt();
	~Instr_sqlrdwrtxt() override;
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
	~Instr_portout() override;
	FrmlElem* IsWord = nullptr;
	FrmlElem* Port = nullptr;
	FrmlElem* PortWhat = nullptr;
};
