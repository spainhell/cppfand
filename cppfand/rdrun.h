#pragma once
#include "access.h"
#include "AddD.h"
#include "FieldDescr.h"
#include "legacy.h"
#include "models/FrmlElem.h"
#include "../Indexes/XScan.h"


struct LvDescr;
class Instr;
class Instr_proc;
struct EdExitD;
class FieldDescr;

enum MInstrCode { _zero, _move, _output, _locvar, _parfile, _ifthenelseM };

struct AssignD : public Chained<AssignD>
{
	MInstrCode Kind = _zero;
	FieldDescr* inputFldD = nullptr;
	FieldDescr* outputFldD = nullptr;
	BYTE* ToPtr = nullptr;
	BYTE* FromPtr = nullptr; 
	WORD L = 0;
	bool Add = false; FrmlElem* Frml = nullptr; FieldDescr* OFldD = nullptr;
	bool Add1 = false; FrmlElem* Frml1 = nullptr; LocVar* LV = nullptr;
	bool Add2 = false; FrmlElem* Frml2 = nullptr;
	FileD* FD = nullptr; FieldDescr* PFldD = nullptr;
	FrmlElem* Bool = nullptr;
	std::vector<AssignD*> Instr;
	std::vector<AssignD*> ElseInstr;
};

struct OutpFD : public Chained<OutpFD>
{
	FileD* FD = nullptr;
	LockMode Md = NullMode;
	void* RecPtr = nullptr;
	FileD* InplFD = nullptr;
	bool Append = false;
#ifdef FandSQL
	SQLStreamPtr Strm;
#endif
};

struct OutpRD : public Chained<OutpRD>
{
	OutpFD* OD = nullptr; /*nullptr=dummy*/
	FrmlElem* Bool = nullptr;
	std::vector<AssignD*> Ass;
};

struct ConstListEl
{
	std::string S;
	double R = 0;
	bool B = false;
};

struct InpD
{
	XScan* Scan = nullptr;
	bool AutoSort = false;
	KeyFldD* SK = nullptr;
	LockMode Md = NullMode;
	longint IRec = 0;
	void* ForwRecPtr = nullptr;
	FrmlElem* Bool = nullptr;
	bool SQLFilter = false;
	KeyFldD* MFld = nullptr;
	std::vector<FrmlElemSum*> *Sum = nullptr;
	bool Exist = false;
	char Op = '\0';
	double Count = 0.0;
	ChkD* Chk = nullptr;
	char OpErr = '\0';
	bool Error = false;
	char OpWarn = '\0';
	bool Warning = false;
	FrmlElem4* ErrTxtFrml = nullptr;
	KeyFldD* SFld = nullptr;                /* only Report */
	std::vector<ConstListEl> OldSFlds;
	LvDescr* FrstLvS = nullptr;
	LvDescr* LstLvS = nullptr;		/* FrstLvS->Ft=DE */
	bool IsInplace = false;              /* only Merge */
	OutpRD* RD = nullptr;
};

/* Report */

enum AutoRprtMode { _ALstg, _ARprt, _ATotal, _AErrRecs };
struct RprtFDListEl
{
	RprtFDListEl* Chain;
	FileD* FD = nullptr;
	XKey* ViewKey = nullptr;
	FrmlElem* Cond = nullptr;
	KeyInD* KeyIn = nullptr;
	bool SQLFilter = false;
	void* LVRecPtr = nullptr;
};

struct RprtOpt
{
	RprtFDListEl FDL;
	std::string Path;
	WORD CatIRec = 0;
	bool UserSelFlds = false, UserCondQuest = false, FromStr = false, SyntxChk = false;
	FrmlElem* Times = nullptr;
	AutoRprtMode Mode;
	RdbPos RprtPos;
	std::vector<FieldDescr*> Flds;  // !empty => autoreport
	std::vector<FieldDescr*> Ctrl;
	std::vector<FieldDescr*> Sum;
	KeyFldD* SK = nullptr;
	FrmlElem* WidthFrml = nullptr, *Head = nullptr;
	WORD Width = 0;
	std::string CondTxt;
	std::string HeadTxt;
	char Style = '\0';
	bool Edit = false, PrintCtrl = false;
};

struct RFldD : public Chained<RFldD>
{
	char FrmlTyp = '\0';    
	char Typ = '\0'; /*R,F,D,T*/
	bool BlankOrWrap = false; /*long date "DD.MM.YYYY"*/
	FrmlElem* Frml = nullptr;
	std::string Name; /*curr. length*/
};

struct BlkD : public Chained<BlkD>
{
	FrmlElem* Bool = nullptr;
	std::vector<FrmlElemSum*> *Sum = nullptr;
	size_t lineLength = 0; // total length of line after printing this block
	bool AbsLine = false, SetPage = false, NotAtEnd = false, FF1 = false, FF2 = false;
	FrmlElem* LineBound = nullptr;
	FrmlElem* LineNo = nullptr;
	FrmlElem* PageNo = nullptr;
	WORD NTxtLines = 0;
	WORD NBlksFrst = 0; // pozice 1. bloku na radku; pred nim jsou mezery
	WORD DHLevel = 0;
	std::vector<RFldD*> ReportFields; // vektor jednotlivych poli formulare
	std::vector<AssignD*> BeforeProc; // prikazy provedene pred blokem
	std::vector<AssignD*> AfterProc; // prikazy provedene po bloku
	std::vector<std::string> lines; // vektor jednotlivych radku
};

struct LvDescr {
	LvDescr* Chain = nullptr;
	LvDescr* ChainBack = nullptr;
	std::vector<FrmlElemSum*> ZeroLst;
	BlkD* Hd = nullptr; 
	BlkD* Ft = nullptr;
	FieldDescr* Fld = nullptr;
};

struct EdExKeyD
{
	std::string KeyName;
	BYTE Break = 0;
	unsigned short KeyCode = 0;
};

struct EdExitD
{
	std::vector<EdExKeyD> Keys;
	bool AtWrRec = false, AtNewRec = false, NegFlds = false;
	//FieldListEl* Flds = nullptr;    /*in edittxt !used*/
	std::vector<FieldDescr*> Flds;    /*in edittxt !used*/
	char Typ = 0;
	void* RO = nullptr;
	Instr_proc* Proc = nullptr;       /*in edittxt only "P","Q"*/
	/*"Q" quit   #0 dummy*/
};

struct EFldD : public Chained<EFldD>
{
	EFldD* ChainBack = nullptr;
	FieldDescr* FldD = nullptr;
	ChkD* Chk = nullptr;
	FrmlElem* Impl = nullptr;
	DepD* Dep = nullptr;
	KeyListEl* KL = nullptr;
	BYTE Page = 0, Col = 0, Ln = 0, L = 0;
	WORD ScanNr = 0;
	bool Tab = false, Dupl = false, Used = false;
	bool EdU = false, EdN = false;
	bool Ed(bool IsNewRec);
};

struct ERecTxtD : public Chained<ERecTxtD>
{
	WORD N;
	StringList SL;
};

struct EditD : Chained<EditD>
{
	// EditD* PrevE; - toto bude pChain ...
	FileD* FD = nullptr;
	LockMode OldMd = NullMode;
	bool IsUserForm = false;
	//FieldListEl* Flds = nullptr;
	std::vector<FieldDescr*> Flds;
	void* OldRecPtr = nullptr; void* NewRecPtr = nullptr;
	BYTE FrstCol = 0, FrstRow = 0, LastCol = 0, LastRow = 0, Rows = 0;
	WRect V; BYTE ShdwX = 0, ShdwY = 0;
	BYTE Attr = 0, dNorm = 0, dHiLi = 0, dSubSet = 0;
	BYTE dDel = 0, dTab = 0, dSelect = 0;
	std::string Top;
	BYTE WFlags = 0;                             /* copied from EO */
	std::vector<EdExitD*> ExD;                   /*      "         */
	FileD* Journal = nullptr;                    /*      "         */
	std::string ViewName;                        /*      "         */
	char OwnerTyp = '\0'; /* #0=CtrlF7 */        /*      "         */
	LinkD* DownLD = nullptr;                     /*      "         */
	LocVar* DownLV = nullptr;                    /*      "         */
	void* DownRecPtr; void* LVRecPtr;            /*      "         */
	KeyInD* KIRoot = nullptr;                    /*      "         */
	bool SQLFilter = false;                      /*      "         */
	XWKey* SelKey = nullptr;                     /*      "         */
	StringListEl* HdTxt = nullptr; BYTE NHdTxt = 0;
	WORD SaveAfter = 0, WatchDelay = 0, RefreshDelay = 0;
	BYTE RecNrPos = 0, RecNrLen = 0;
	BYTE NPages = 0;
	//std::vector<std::string> RecTxt;
	ERecTxtD* RecTxt = nullptr;
	BYTE NRecs = 0;     /*display*/
	EFldD* FirstFld = nullptr;
	EFldD* LastFld = nullptr; 
	EFldD* StartFld = nullptr;
	EFldD* CFld = nullptr; EFldD* FirstEmptyFld = nullptr;    /*copied*/
	XKey* VK = nullptr; XWKey* WK = nullptr;                  /*  "   */
	longint BaseRec = 0; BYTE IRec = 0;                       /*  "   */
	bool IsNewRec = false, Append = false, Select = false,    /*  "   */
		 WasUpdated = false, EdRecVar = false,                /*  "   */
		 AddSwitch = false, ChkSwitch = false,                /*  "   */
		 WarnSwitch = false, SubSet = false;                  /*  "   */
	bool NoDelTFlds = false, WasWK = false;                   /*  "   */
	bool NoDelete = false, VerifyDelete = false,              /*  "   */
	     NoCreate = false, F1Mode = false,                    /*  "   */
		 OnlyAppend = false, OnlySearch = false,              /*  "   */
	     Only1Record = false, OnlyTabs = false,               /*  "   */
		 NoESCPrompt = false, MustESCPrompt = false,          /*  "   */
	     Prompt158 = false, NoSrchMsg = false,                /*  "   */
		 WithBoolDispl = false, Mode24 = false,               /*  "   */
	     NoCondCheck = false, F3LeadIn = false,               /*  "   */
		 LUpRDown = false, MouseEnter = false,                /*  "   */
	     TTExit = false,                                      /*  "   */
		 MakeWorkX = false, NoShiftF7Msg = false,             /*  "   */
	     MustAdd = false;                                     /*  "   */
	bool MustCheck = false, SelMode = false;                  /*  "   */
	bool DownSet = false, IsLocked = false, WwPart = false;    
	XKey* DownKey = nullptr;
	longint LockedRec = 0;
	FrmlElem* Cond = nullptr; FrmlElem* Bool = nullptr;
	std::string BoolTxt;
	std::string Head; std::string Last;
	std::string CtrlLast; std::string AltLast; std::string ShiftLast;
	WORD NFlds = 0, NTabsSet = 0, NDuplSet = 0, NEdSet = 0;
	bool EdUpdated = false;
	ImplD* Impl = nullptr;
	longint StartRecNo = 0;
	std::string StartRecKey;
	integer StartIRec = 0;
	longint OwnerRecNo = 0;
	LinkD* ShiftF7LD = nullptr;
	void* AfterE = nullptr;
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

enum class CpOption {cpNo, cpFix, cpVar, cpTxt};

struct CopyD
{
	// pstring* Path1; /*FrmlPtr if cpList*/
	std::string Path1;
	WORD CatIRec1;
	FileD* FD1;
	XKey* ViewKey;
	bool WithX1;
	CpOption Opt1;
	// pstring* Path2; /*  "  */
	std::string Path2;
	WORD CatIRec2;
	FileD* FD2;
	bool WithX2;
	CpOption Opt2;
	FileD* HdFD;
	FieldDescr* HdF;
	bool Append, NoCancel;
	BYTE Mode;
};

struct ChoiceD
{
	std::string HelpName;
	bool Displ = false, DisplEver = false, Enabled = false, TxtConst = false;
	FrmlElem* Condition = nullptr;
	Instr* Instr = nullptr;
	FrmlElem* TxtFrml = nullptr;
	std::string Txt;
};

struct WrLnD : Chained<WrLnD>
{
	FrmlElem* Frml = nullptr;
	char Typ = '\0'; /* S, B, F, D */
	BYTE N = 0, M = 0;
	std::string* Mask = nullptr;
};

struct LockD
{
	LockD* Chain = nullptr;
	FileD* FD = nullptr;
	FrmlElem* Frml = nullptr;
	LockMode Md, OldMd;
	longint N = 0;
};

struct TypAndFrml
{
	char FTyp = '\0';
	FrmlElem* Frml = nullptr; 
	bool FromProlog = false, IsRetPar = false;
	FileD* FD = nullptr; 
	void* RecPtr = nullptr;
	FrmlElem* TxtFrml = nullptr; 
	std::string Name; // if RecPtr != nullptr
};

class Instr
{
public:
	Instr(PInstrCode kind);
	PInstrCode Kind;
	Instr* Chain = nullptr;
};

extern std::vector<ConstListEl> OldMFlds;
extern std::vector<ConstListEl> NewMFlds;   /* Merge + Report*/
extern InpD* IDA[30];
extern integer MaxIi;
extern XString OldMXStr;                  /* Merge */
extern OutpFD* OutpFDRoot;
extern OutpRD* OutpRDs;
extern bool Join;
extern bool PrintView;                  /* Report */
extern TextFile Rprt;		// puvodne text - souvisi s text. souborem
extern BlkD* RprtHd;
extern BlkD* PageHd;
extern BlkD* PageFt;
extern std::vector<FrmlElemSum*> PFZeroLst;
extern LvDescr* FrstLvM;
extern LvDescr* LstLvM; /* LstLvM->Ft=RF */
extern bool SelQuest;
	/* Edit */
extern FrmlElem* PgeSizeZ, *PgeLimitZ;
extern EditD* EditDRoot;
extern bool CompileFD, EditRdbMode;
extern LocVarBlkD LVBD;

extern std::string CalcTxt;
struct MergOpSt { char Op; double Group; };
extern MergOpSt MergOpGroup;

// *** IMPLEMENTATION ***

void ResetLVBD();
bool RunAddUpdte1(char Kind/*+,-,d*/, void* CRold, bool Back/*tracking*/, AddD* StopAD, LinkD* notLD);

void CrIndRec();
bool Link(AddD* AD, longint& N, char& Kind2);
bool TransAdd(AddD* AD, FileD* FD, void* RP, void* CRnew, longint N, char Kind2, bool Back);
void WrUpdRec(AddD* AD, FileD* FD, void* RP, void* CRnew, longint N);
bool Assign(AddD* AD);
bool LockForAdd(FileD* FD, WORD Kind, bool Ta, LockMode& md);
bool RunAddUpdte(char Kind, void* CRold, LinkD* notLD);
bool TestExitKey(WORD KeyCode, EdExitD* X);
void SetCompileAll();
