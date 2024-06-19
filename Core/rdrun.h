#pragma once
#include <memory>

#include "access.h"
#include "FieldDescr.h"
#include "legacy.h"
#include "../DataEditor/EditD.h"
#include "models/FrmlElem.h"
#include "../fandio/XScan.h"


struct LvDescr;
class Instr;
class Instr_proc;
struct EdExitD;
class FieldDescr;

enum class MInstrCode { _zero, _move, _output, _locvar, _parfile, _ifThenElse };

struct AssignD : public Chained<AssignD>
{
	MInstrCode Kind = MInstrCode::_zero;
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
	int IRec = 0;
	void* ForwRecPtr = nullptr;
	FrmlElem* Bool = nullptr;
	bool SQLFilter = false;
	KeyFldD* MFld = nullptr;
	std::vector<FrmlElemSum*> *Sum = nullptr;
	bool Exist = false;
	char Op = '\0';
	double Count = 0.0;
	std::vector<ChkD*> Chk;
	char OpErr = '\0';
	bool Error = false;
	char OpWarn = '\0';
	bool Warning = false;
	FrmlElemString* ErrTxtFrml = nullptr;
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
	char Typ = '\0'; /*rdb,F,D,T*/
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
	std::vector<ChkD*> Chk;
	FrmlElem* Impl = nullptr;
	std::vector<DepD*> Dep;
	std::vector<XKey*> KL;
	BYTE Page = 0, Col = 0, Ln = 0, L = 0;
	WORD ScanNr = 0;
	bool Tab = false, Dupl = false, Used = false;
	bool EdU = false, EdN = false;
	bool Ed(bool IsNewRec);
};

struct ERecTxtD : public Chained<ERecTxtD>
{
	WORD N;
	std::vector<std::string> SL;
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
	std::vector<Instr*> v_instr;
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
	int N = 0;
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


extern std::vector<ConstListEl> OldMFlds;
extern std::vector<ConstListEl> NewMFlds;   /* Merge + Report*/
extern InpD* IDA[30];
extern short MaxIi;
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

void ResetLVBD();
void CrIndRec(FileD* file_d, void* record);
bool Link(FileD* file_d, AddD* add_d, int& n, char& kind2, void* record, BYTE** linkedRecord);
bool TransAdd(FileD* file_d, AddD* AD, FileD* FD, void* RP, void* new_record, int N, char Kind2, bool Back);
void WrUpdRec(FileD* file_d, AddD* add_d, FileD* fd, void* rp, void* new_record, int n);
bool Assign(FileD* file_d, AddD* add_d, void* record);
bool LockForAdd(FileD* file_d, WORD kind, bool Ta, LockMode& md);

bool RunAddUpdate(FileD* file_d, char kind, void* old_record, LinkD* not_link_d, void* record);

/**
 * \brief What does it do?
 * \param file_d file
 * \param kind +, -, d
 * \param old_record old record
 * \param back backtracking
 * \param stop_add_d stop AddD object
 * \param not_link_d not LinkD
 * \param record record
 * \return bool
 */
bool RunAddUpdate(FileD* file_d, char kind, void* old_record, bool back, AddD* stop_add_d, LinkD* not_link_d, void* record);
bool TestExitKey(WORD KeyCode, EdExitD* X);
void SetCompileAll();
