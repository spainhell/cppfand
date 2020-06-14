#pragma once
#include "access.h"
#include "editor.h"
#include "legacy.h"


struct LvDescr;
struct Instr;
struct EdExitD;

enum MInstrCode { _zero, _move, _output, _locvar, _parfile, _ifthenelseM };
struct AssignD : public Chained
{
	//AssignD* Chain;
	MInstrCode Kind = _zero;
	FieldDescr* FldD = nullptr;
	BYTE* ToPtr = nullptr;
	BYTE* FromPtr = nullptr; 
	WORD L = 0;
	bool Add = false; FrmlElem* Frml = nullptr; FieldDescr* OFldD = nullptr;
	bool Add1 = false; FrmlElem* Frml1 = nullptr; LocVar* LV = nullptr;
	bool Add2 = false; FrmlElem* Frml2 = nullptr;
	FileD* FD = nullptr; FieldDescr* PFldD = nullptr;
	FrmlElem* Bool = nullptr;
	AssignD* Instr = nullptr;
	AssignD* ElseInstr = nullptr;
};

struct OutpFD : public Chained
{
	//OutpFD* Chain;
	FileDPtr FD;
	LockMode Md;
	void* RecPtr;
	FileDPtr InplFD;
	bool Append;
#ifdef FandSQL
	SQLStreamPtr Strm;
#endif
};

struct OutpRD : public Chained
{
	//OutpRD* Chain;
	OutpFD* OD; /*nullptr=dummy*/
	FrmlPtr Bool;
	AssignD* Ass;
};

struct ConstListEl : public Chained
{
	pstring S = "";
	double R = 0;
	bool B = false;
};

struct InpD
{
	XScan* Scan;
	bool AutoSort;
	KeyFldDPtr SK;
	LockMode Md;
	longint IRec;
	void* ForwRecPtr;
	FrmlPtr Bool;
	bool SQLFilter;
	KeyFldDPtr MFld;
	SumElPtr Sum;
	bool Exist;
	char Op;
	double Count;
	ChkDPtr Chk;
	char OpErr;
	bool Error;
	char OpWarn;
	bool Warning;
	FrmlPtr ErrTxtFrml;
	KeyFldDPtr SFld;                /* only Report */
	ConstListEl* OldSFlds;
	LvDescr* FrstLvS;
	LvDescr* LstLvS;		/* FrstLvS->Ft=DE */
	bool IsInplace;              /* only Merge */
	OutpRD* RD;
};

/* Report */

enum AutoRprtMode { _ALstg, _ARprt, _ATotal, _AErrRecs };
struct RprtFDListEl
{
	RprtFDListEl* Chain; FileDPtr FD; KeyDPtr ViewKey;
	FrmlPtr Cond; KeyInD* KeyIn; bool SQLFilter;
	void* LVRecPtr;
};

struct RprtOpt
{
	RprtFDListEl FDL;
	pstring* Path;
	WORD CatIRec;
	bool UserSelFlds, UserCondQuest, FromStr, SyntxChk;
	FrmlPtr Times;
	AutoRprtMode Mode;
	RdbPos RprtPos;
	FieldList Flds;		 /* != nullptr => autoreport*/
	FieldList Ctrl;
	FieldList Sum;
	KeyFldDPtr SK;
	FrmlPtr WidthFrml, Head;
	WORD Width;
	pstring* CondTxt;
	LongStr* HeadTxt;
	char Style;
	bool Edit, PrintCtrl;
};

struct RFldD : public Chained
{
	//RFldD* Chain;
	char FrmlTyp, Typ;    /*R,F,D,T*/
	bool BlankOrWrap; /*long date "DD.MM.YYYY"*/
	FrmlPtr Frml;
	pstring Name = pstring(1); /*curr. length*/
};

struct BlkD : public Chained
{
	// BlkD* Chain;
	FrmlElem* Bool = nullptr;
	SumElem* Sum = nullptr;
	char* Txt = nullptr;          /*sequence of pstrings*/
	bool AbsLine = false, SetPage = false, NotAtEnd = false, FF1 = false, FF2 = false;
	FrmlElem* LineBound = nullptr;
	FrmlElem* LineNo = nullptr;
	FrmlElem* PageNo = nullptr;
	WORD NTxtLines = 0, NBlksFrst = 0, DHLevel = 0;
	RFldD* RFD = nullptr;
	AssignD* BeforeProc = nullptr; 
	AssignD* AfterProc = nullptr;
};

struct LvDescr {
	LvDescr* Chain = nullptr;
	LvDescr* ChainBack = nullptr;
	FloatPtrListEl* ZeroLst = nullptr;
	BlkD* Hd = nullptr; 
	BlkD* Ft = nullptr;
	FieldDescr* Fld = nullptr;
};

struct EdExKeyD
{
	EdExKeyD* Chain = nullptr;
	BYTE Break = 0;
	WORD KeyCode = 0;
};

struct EdExitD : Chained
{
	//EdExitD* Chain = nullptr;
	EdExKeyD* Keys = nullptr;;
	bool AtWrRec = false, AtNewRec = false, NegFlds = false;
	FieldList Flds = nullptr;    /*in edittxt !used*/
	char Typ = 0;
	void* RO = nullptr;
	Instr* Proc = nullptr;       /*in edittxt only "P","Q"*/
	/*"Q" quit   #0 dummy*/
};

struct EditOpt
{
	RdbPos FormPos;
	bool UserSelFlds = false, SetOnlyView = false, NegDupl = false;
	bool NegTab = false, NegNoEd = false, SyntxChk = false;
	FieldListEl* Flds = nullptr; FieldListEl* Dupl = nullptr; 
	FieldListEl* Tab = nullptr; FieldListEl* NoEd = nullptr;
	FrmlElem* Cond = nullptr;
	FrmlElem* Head = nullptr; FrmlElem* Last = nullptr; 
	FrmlElem* CtrlLast = nullptr; FrmlElem* AltLast = nullptr; FrmlElem* ShiftLast = nullptr;
	FrmlElem* Mode = nullptr;
	FrmlElem* StartRecNoZ = nullptr; FrmlElem* StartRecKeyZ = nullptr;
	FrmlElem* StartIRecZ = nullptr; FrmlElem* StartFieldZ = nullptr;
	FrmlElem* SaveAfterZ = nullptr; FrmlElem* WatchDelayZ = nullptr; 
	FrmlElem* RefreshDelayZ = nullptr;
	WRectFrml W;
	FrmlElem* ZAttr = nullptr;
	FrmlElem* ZdNorm = nullptr;
	FrmlElem* ZdHiLi = nullptr;
	FrmlElem* ZdSubset = nullptr;
	FrmlElem* ZdDel = nullptr;
	FrmlElem* ZdTab = nullptr;
	FrmlElem* ZdSelect = nullptr;
	FrmlElem* Top = nullptr;
	BYTE WFlags = 0;
	EdExitD* ExD = nullptr;
	FileD* Journal = nullptr;
	pstring* ViewName = nullptr;
	char OwnerTyp= '\0';
	LinkD* DownLD = nullptr;
	LocVar* DownLV = nullptr;
	void* DownRecPtr = nullptr; void* LVRecPtr = nullptr;
	KeyInD* KIRoot = nullptr;
	bool SQLFilter = false;
	XKey* SelKey = nullptr; 
	XKey* ViewKey = nullptr;
};

struct EFldD : public Chained
{
	//EFldD* Chain;
	EFldD* ChainBack;
	FieldDPtr FldD;
	ChkDPtr Chk;
	FrmlPtr Impl;
	DepDPtr Dep;
	KeyList KL;
	BYTE Page, Col, Ln, L;
	WORD ScanNr;
	bool Tab, Dupl, Used, EdU, EdN;
	bool Ed(bool IsNewRec);
};

struct ERecTxtD : public Chained
{
	WORD N;
	StringList SL;
};

struct EditD : Chained
{
	// EditD* PrevE; - toto bude Chain ...
	FileD* FD;
	LockMode OldMd;
	bool IsUserForm;
	FieldList Flds;
	void* OldRecPtr; void* NewRecPtr;
	BYTE FrstCol, FrstRow, LastCol, LastRow, Rows;
	WRect V; BYTE ShdwX, ShdwY;
	BYTE Attr, dNorm, dHiLi, dSubSet, dDel, dTab, dSelect;
	pstring* Top = nullptr;
	BYTE WFlags;                                 /* copied from EO */
	EdExitD* ExD = nullptr;                      /*      "         */
	FileD* Journal = nullptr;                    /*      "         */
	pstring* ViewName;                           /*      "         */
	char OwnerTyp; /* #0=CtrlF7 */               /*      "         */
	LinkD* DownLD = nullptr;                     /*      "         */
	LocVar* DownLV = nullptr;                    /*      "         */
	void* DownRecPtr; void* LVRecPtr;            /*      "         */
	KeyInD* KIRoot = nullptr;                    /*      "         */
	bool SQLFilter;                              /*      "         */
	XWKey* SelKey = nullptr;                     /*      "         */
	StringList HdTxt; BYTE NHdTxt;
	WORD SaveAfter, WatchDelay, RefreshDelay;
	BYTE RecNrPos, RecNrLen;
	BYTE NPages;
	ERecTxtD* RecTxt = nullptr;
	BYTE NRecs;     /*display*/
	EFldD* FirstFld = nullptr;
	EFldD* LastFld = nullptr; 
	EFldD* StartFld = nullptr;
	EFldD* CFld = nullptr; EFldD* FirstEmptyFld = nullptr;    /*copied*/
	KeyDPtr VK = nullptr; WKeyDPtr WK = nullptr;              /*  "   */
	longint BaseRec; BYTE IRec;                               /*  "   */
	bool IsNewRec, Append, Select, WasUpdated, EdRecVar,      /*  "   */
		AddSwitch, ChkSwitch, WarnSwitch, SubSet;             /*  "   */
	bool NoDelTFlds, WasWK;                                   /*  "   */
	bool NoDelete, VerifyDelete, NoCreate, F1Mode,            /*  "   */
		OnlyAppend, OnlySearch, Only1Record, OnlyTabs,        /*  "   */
		NoESCPrompt, MustESCPrompt, Prompt158, NoSrchMsg,     /*  "   */
		WithBoolDispl, Mode24, NoCondCheck, F3LeadIn,         /*  "   */
		LUpRDown, MouseEnter, TTExit,                         /*  "   */
		MakeWorkX, NoShiftF7Msg, MustAdd;                     /*  "   */
	bool MustCheck, SelMode;                                  /*  "   */
	bool DownSet, IsLocked, WwPart;
	KeyDPtr DownKey;
	longint LockedRec;
	FrmlPtr Cond, Bool;
	pstring* BoolTxt, Head, Last, CtrlLast, AltLast, ShiftLast;
	WORD NFlds, NTabsSet, NDuplSet, NEdSet;
	bool EdUpdated;
	ImplD* Impl;
	longint StartRecNo; pstring* StartRecKey; integer StartIRec;
	longint OwnerRecNo;
	LinkD* ShiftF7LD;
	void* AfterE;
};

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

enum CpOption {cpNo, cpFix, cpVar, cpTxt};

struct CopyD
{
	pstring* Path1; /*FrmlPtr if cpList*/
	WORD CatIRec1;
	FileDPtr FD1;
	KeyDPtr ViewKey;
	bool WithX1;
	CpOption Opt1;
	pstring* Path2; /*  "  */
	WORD CatIRec2;
	FileDPtr FD2;
	bool WithX2;
	CpOption Opt2;
	FileDPtr HdFD;
	FieldDPtr HdF;
	bool Append, NoCancel;
	BYTE Mode;
};

struct ChoiceD : Chained
{
	//ChoiceD* Chain = nullptr;
	pstring* HelpName = nullptr;
	bool Displ = false, DisplEver = false, Enabled = false, TxtConst = false;
	FrmlPtr Bool = nullptr;
	Instr* Instr = nullptr;
	FrmlPtr TxtFrml = nullptr;
	pstring* Txt = nullptr;
};

struct WrLnD : public Chained
{
	//WrLnD* Chain;
	FrmlElem* Frml = nullptr;
	char Typ = '\0'; /*S,B,F,D*/
	BYTE N = 0, M = 0;
	pstring* Mask = nullptr;
};

struct LockD
{
	LockD* Chain = nullptr;
	FileD* FD = nullptr;
	FrmlElem* Frml = nullptr;
	LockMode Md, OldMd;
	longint N = 0;
};

struct GraphVD : public Chained
{
	//GraphVD* Chain;
	FrmlPtr XZ, YZ, Velikost; /*float*/
	FrmlPtr BarPis, Text; /*pstring*/
};

struct GraphWD : public Chained
{
	//GraphWD* Chain;
	FrmlPtr XZ, YZ, XK, YK; /*float*/
	FrmlPtr BarPoz, BarPis, Text; /*pstring*/
};

struct GraphRGBD : public Chained
{
	// GraphRGBD* Chain;
	FrmlPtr Barva; /*pstring*/
	FrmlPtr R, G, B; /*float*/
};

struct WinG
{
	WRectFrml W;
	WRect WR;
	FrmlPtr ColFrame, ColBack, ColFor;  /*pstring*/
	FrmlPtr Top;
	BYTE WFlags;
};

struct GraphD
{
	FileDPtr FD;
	FrmlPtr GF;
	FieldDPtr X, Y, Z;
	FieldDPtr ZA[10];
	FrmlPtr HZA[10];
	FrmlPtr T, H, HX, HY, HZ, C, D, R, P, CO, Assign, Cond; /*pstring*/
	FrmlPtr S, RS, RN, Max, Min, SP; /*float*/
	bool Interact;
	GraphVD* V;
	GraphWD* W;
	GraphRGBD* RGB;
	KeyInD* KeyIn;
	bool SQLFilter;
	KeyDPtr ViewKey;
	WinG* WW;
};

struct TypAndFrml
{
	char FTyp;
	FrmlPtr Frml; bool FromProlog, IsRetPar;
	FileDPtr FD; void* RecPtr;
	FrmlPtr TxtFrml; pstring Name; // if RecPtr != nullptr
};

struct Instr : public Chained// POZOR konflikt názvù viz níže
{
	//Instr* Chain = nullptr;
	PInstrCode Kind;
	FrmlPtr HdLine;
	RdbDPtr HelpRdb;
	bool WasESCBranch;
	Instr* ESCInstr = nullptr;
	ChoiceD* Choices = nullptr;
	bool Loop, PullDown, Shdw;
	FrmlPtr X, Y, XSz;
	FrmlPtr mAttr[4];
	FrmlPtr Bool;
	Instr* Instr1 = nullptr;
	Instr* ElseInstr1 = nullptr;  // pùvodnì Instr a ElseInstr -> konflikt názvù
	RdbPos Pos;
	RdbPos PPos;
	BYTE N;
	bool ExPar;
	TypAndFrml TArg[2];
	RdbPos lpPos;
	pstring* lpName = nullptr;
	pstring* RdbNm = nullptr;
	pstring* ProcNm = nullptr;
	Instr* ProcCall = nullptr;
	pstring* ProgPath = nullptr;
	WORD ProgCatIRec;
	bool NoCancel, FreeMm, LdFont, TextMd;
	FrmlPtr Param;
	CopyD* CD = nullptr;
	BYTE LF /*0-write,1-writeln,2-message,3-message+help*/;
	WrLnD WD;
	RdbDPtr mHlpRdb = nullptr;
	FrmlPtr mHlpFrml = nullptr;
	FrmlPtr GoX, GoY;
	FrmlPtr Frml = nullptr;
	bool Add;
	LocVar* AssLV;
	FrmlPtr Frml0 = nullptr;
	RdbDPtr HelpRdb0 = nullptr;
	FrmlPtr Frml1 = nullptr;
	bool Add1;
	FileDPtr FD = nullptr;
	FieldDPtr FldD = nullptr;
	FrmlPtr RecFrml = nullptr;
	bool Indexarg;
	FrmlPtr Frml2 = nullptr; bool Add2; LocVar* AssLV2 = nullptr; FieldDPtr RecFldD = nullptr;
	FrmlPtr Frml3 = nullptr; FileDPtr FD3 = nullptr; WORD CatIRec; FieldDPtr CatFld = nullptr;
	LocVar* RecLV1 = nullptr; LocVar* RecLV2 = nullptr;
	AssignD* Ass;
	LinkDPtr LinkLD = nullptr;
	WKeyDPtr xnrIdx;
	FrmlPtr RecNr; bool AdUpd;
	LocVar* LV; bool ByKey; KeyDPtr Key;
	char CompOp;
	FileDPtr RecFD;
	FileDPtr NextGenFD;
	WORD FrstCatIRec, NCatIRecs; FrmlPtr TCFrml;
	FileDPtr SortFD = nullptr; KeyFldDPtr SK = nullptr;
	FileDPtr EditFD = nullptr; EditOpt* EO = nullptr;
	RprtOpt* RO = nullptr;
	pstring* TxtPath = nullptr; WORD TxtCatIRec; LocVar* TxtLV = nullptr;
	char EdTxtMode;
	EdExitD* ExD = nullptr;
	BYTE WFlags; FrmlPtr TxtPos, TxtXY, ErrMsg;
	WRectFrml Ww; FrmlPtr Atr; FrmlPtr Hd;
	FrmlPtr Head, Last, CtrlLast, AltLast, ShiftLast;
	pstring* TxtPath1 = nullptr; WORD TxtCatIRec1;
	FrmlPtr Txt; bool App;
	FrmlPtr Drive;
	WORD MountCatIRec; bool MountNoCancel;
	FileDPtr IndexFD; bool Compress;
	LocVar* giLV; char giMode; /*+,-,blank*/
	FrmlPtr giCond; /* || RecNr-Frml */
	KeyDPtr giKD; KeyFldDPtr giKFlds;
	KeyInD* giKIRoot; bool giSQLFilter;
	char giOwnerTyp; LinkDPtr giLD; LocVar* giLV2;
	WRectFrml W; FrmlPtr Attr; Instr* WwInstr; FrmlPtr Top;
	BYTE WithWFlags;
	WRectFrml W2; FrmlPtr Attr2, FillC;
	FileDPtr CFD; KeyDPtr CKey; LocVar* CVar, CRecVar;
	KeyInD* CKIRoot; FrmlPtr CBool/*or SQLTxt*/; Instr* CInstr;
	LinkDPtr CLD; bool CWIdx, inSQL, CSQLFilter, CProcent;
	char COwnerTyp; LocVar* CLV;
	Instr* WDoInstr; Instr* WElseInstr; bool WasElse; LockD WLD;
	GraphD* GD;
	FrmlPtr Par1, Par2, Par3, Par4, Par5, Par6, Par7, Par8, Par9, Par10, Par11;
	WORD BrCatIRec; bool IsBackup, NoCompress, BrNoCancel;
	BYTE bmX[5];
	FrmlPtr bmDir; FrmlPtr bmMasks; /*backup only*/
	bool bmSubDir, bmOverwr;
	FileDPtr clFD;
	FrmlPtr Insert, Indent, Wrap, Just, ColBlk, Left, Right;
	FrmlPtr MouseX, MouseY, Show;
	FileDPtr cfFD; pstring* cfPath; WORD cfCatIRec;
	FrmlPtr liName, liPassWord;
	pstring* TxtPath2; WORD TxtCatIRec2; bool IsRead;
	FileDPtr sqlFD; KeyDPtr sqlKey; FieldDPtr sqlFldD; FrmlPtr sqlXStr;
	FrmlPtr IsWord, Port, PortWhat;
};

extern ConstListEl* OldMFlds;
extern ConstListEl* NewMFlds;   /* Merge + Report*/
extern InpD* IDA[9];
extern integer MaxIi;
extern XString OldMXStr;                  /* Merge */
extern OutpFD* OutpFDRoot;
extern OutpRD* OutpRDs;
extern bool Join;
extern bool PrintView;                  /* Report */
extern TextFile Rprt;		// pùvodnì text - souvisí s text. souborem
extern BlkD* RprtHd;
extern BlkD* PageHd;
extern BlkD* PageFt;
extern FloatPtrListEl* PFZeroLst;
extern LvDescr* FrstLvM;
extern LvDescr* LstLvM; /* LstLvM->Ft=RF */
extern bool SelQuest;
extern FrmlPtr PgeSizeZ, PgeLimitZ;
	/* Edit */
extern EditD* EditDRoot;
extern bool CompileFD, EditRdbMode;
extern LocVarBlkD LVBD;

extern pstring CalcTxt;
struct MergOpSt { char Op; double Group; };
extern MergOpSt MergOpGroup;

// *** IMPLEMENTATION ***

void ResetLVBD();
void SetMyBP(ProcStkD* Bp);
void PushProcStk();
void PopProcStk();
bool RunAddUpdte1(char Kind/*+,-,d*/, void* CRold, bool Back/*tracking*/,
	AddDPtr StopAD, LinkDPtr notLD);

void CrIndRec();
bool Link(AddD* AD, longint& N, char& Kind2);
bool TransAdd(AddD* AD, FileD* FD, void* RP, void* CRnew, longint N, char Kind2, bool Back);
bool Add(AddD* AD, void* RP, double R);
void WrUpdRec(AddD* AD, FileD* FD, void* RP, void* CRnew, longint N);
bool Assign(AddDPtr AD);
bool LockForAdd(FileDPtr FD, WORD Kind, bool Ta, LockMode& md);
bool RunAddUpdte(char Kind, void* CRold, LinkD* notLD);
bool TestExitKey(WORD KeyCode, EdExitD* X);
void SetCompileAll();
