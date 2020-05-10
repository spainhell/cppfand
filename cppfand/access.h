#pragma once

#include "base.h"
#include "constants.h"
#include "globconf.h"
#include "runfand.h"
#ifdef FandSQL
#include "channel.h"
#endif

struct FrmlListEl;
class XFile;
struct FrmlElem;
struct LocVar;
struct FuncD;
class XWKey;
class XWFile;
class XKey;
struct LinkD;
class FileD;
struct RdbD;
struct FieldDescr;
using namespace std;

// ********** CONST **********
const BYTE LeftJust = 1; // {RightJust=0  coded in M for Typ='N','A'}
const BYTE Ascend = 0; const BYTE Descend = 6; // {used in SortKey}
const WORD XPageSize = 1024; const BYTE XPageOverHead = 7; const BYTE MaxIndexLen = 123; //{ min.4 items };
const BYTE oLeaf = 3; const BYTE oNotLeaf = 7;
const BYTE f_Stored = 1; const BYTE f_Encryp = 2; // {FieldD flags}
const BYTE f_Mask = 4; const BYTE f_Comma = 8; // {FieldD flags}

enum LockMode { NullMode = 0, NoExclMode = 1, NoDelMode = 2, NoCrMode = 3,
	RdMode = 4, WrMode = 5, CrMode = 6, DelMode = 7, ExclMode = 8 };
static pstring LockModeTxt[9] = { "NULL", "NOEXCL","NODEL","NOCR","RD","WR","CR","DEL","EXCL" };

const WORD MPageSize = 512;
const BYTE XPageShft = 10;
const BYTE MPageShft = 9;

typedef char PwCodeArr[20];

typedef XKey* KeyDPtr;
typedef XKey KeyD;
typedef FuncD* FuncDPtr;
typedef XWKey* WKeyDPtr;


struct FieldListEl // r32
{
	FieldListEl* Chain;
	FieldDescr* FldD;
};
typedef FieldListEl* FieldList;

struct FrmlListEl // ш. 34
{
	FrmlListEl* Chain;
	FrmlElem* Frml;
};
typedef FrmlListEl* FrmlList;

struct StringListEl // ш. 38
{
	StringListEl* Chain;
	pstring S;
};
typedef StringListEl* StringList;

struct FloatPtrListEl // r42
{
	FloatPtrListEl* Chain;
	double* RPtr;
};
typedef FloatPtrListEl* FloatPtrList;

struct KeyListEl // ш. 49
{
	KeyListEl* Chain;
	KeyDPtr Key;
};
typedef KeyListEl* KeyList;


struct FrmlElem // ш. 51
{
	BYTE Op;
	FrmlElem* P1; FrmlElem* P2; FrmlElem* P3; FrmlElem* P4; FrmlElem* P5; FrmlElem* P6; // 0
	char Delim; // 0
	BYTE N01, N02, N03, N04, N11, N12, N13, N14, N21, N22, N23, N24, N31; // 1
	BYTE W01, W02, W11, W12, W21, W22; // 1
	double R; // 2
	pstring S; // 4
	bool B; // 5
	FrmlElem* PP1; pstring Mask; // 6
	FieldDescr* Field; // 7 {_field}
	FrmlElem* P011; FileD* File2; LinkD* LD; // 7  {LD=nil for param} {_access} {LD=RecPtr} {_recvarfld}
	FrmlElem* Frml; FileD* NewFile; void* NewRP; // 8 {_newfile}
	FileD* FD; // 9 {_lastupdate, _generation}
	WORD CatIRec; FieldDescr* CatFld; // 10 {_catfield}
	FrmlElem* PPP1; FrmlElem* PP2; FieldDescr* FldD; // 11 {_prompt}
	FrmlElem* PPPP1; FrmlElem* PPP2; FrmlElem* PP3; pstring Options; // 12 {_pos,_replace}
	FileD* FFD; KeyD* Key; FrmlElem* Arg[2]; // 13 {_recno/typ='R' or'S'/,_recnoabs,_recnolog}
	FrmlElem* PPPPP1; FileD* RecFD; FieldDescr* RecFldD; // 14 {_accrecno,_isdeleted}
	LinkD* LinkLD; bool LinkFromRec; LocVar* LinkLV; FrmlElem* LinkRecFrml; // 15 {_link}
	FrmlElem* PPPPPP1; FrmlElem* PPPP2; pstring* TxtPath; WORD TxtCatIRec; // 16 {_gettxt,_filesize}
	WORD BPOfs; // 18 { _getlocvar }
	FuncDPtr FC; FrmlList FrmlL; // 19 { _userfunc }
	LocVar* LV; KeyD* PackKey; // 20 { _keyof,_lvdeleted }
	FrmlElem* EvalP1; char EvalTyp; FileD* EvalFD; // 21 {_eval}
	WKeyDPtr WKey; // 22 {_indexnrecs}
	FrmlElem* ownBool; FrmlElem* ownSum; LinkD* ownLD; // 23 { _owned }
};
typedef FrmlElem* FrmlPtr;

struct KeyInD // r89
{
	KeyInD* Chain;
	FrmlList FL1, FL2;
	longint XNrBeg, N;
	pstring* X1;
	pstring* X2;
};

struct SumElem // r95
{
	SumElem* Chain;
	char Op;
	double R;
	FrmlPtr Frml;
};
typedef SumElem* SumElPtr;

struct FieldDescr // ш. 100
{
	FieldDescr* Chain = nullptr;
	char Typ = 0, FrmlTyp = 0;
	BYTE L = 0, M = 0, NBytes = 0, Flg = 0;
	// case boolean {Stored} of True:(Displ:integer); False:(Frml:FrmlPtr; Name:string[1]{ curr.length });
	integer Displ = 0;
	FrmlElem* Frml = nullptr;
	pstring Name;
};
typedef FieldDescr* FieldDPtr;

struct KeyFldD // ш. 108
{
	KeyFldD* Chain = nullptr;
	FieldDescr* FldD = nullptr;
	bool CompLex = false, Descend = false;
};
typedef KeyFldD* KeyFldDPtr;

struct RdbPos // ш. 113
{
	RdbD* R = nullptr;
	WORD IRec = 0;
};

struct ChkD // ш. 115
{
	ChkD* Chain = nullptr;
	FrmlPtr Bool;
	pstring* HelpName = nullptr;
	FrmlPtr TxtZ;
	bool Warning = false;
};
typedef ChkD* ChkDPtr;

struct DepD // r122
{
	DepD* Chain; FrmlPtr Bool, Frml;
};
typedef DepD* DepDPtr;

struct ImplD
{
	ImplD* Chain; FieldDPtr FldD; FrmlPtr Frml;
};
typedef ImplD* ImplDPtr;

struct LiRoots
{
	ChkD* Chks; ImplD* Impls;
};
typedef LiRoots* LiRootsPtr;

struct AddD // ш. 135
{
	AddD* Chain;
	FieldDPtr Field;
	FileD* File2;
	LinkD* LD;
	BYTE Create; // { 0-no, 1-!, 2-!! }
	FrmlPtr Frml;
	bool Assign;
	FrmlPtr Bool;
	ChkDPtr Chk;
};
typedef AddD* AddDPtr;

class TFile // ш. 147
{
public:
	FILE* Handle;
	longint FreePart;
	bool Reserved, CompileProc, CompileAll;
	WORD IRec;
	longint FreeRoot, MaxPage;
	double TimeStmp;
	integer LicenseNr;
	longint MLen;
	PwCodeArr PwCode, Pw2Code;
	enum eFormat { T00Format, DbtFormat, FptFormat } Format;
	WORD BlockSize; // FptFormat
	bool IsWork;
	void Err(WORD n, bool ex);
	void TestErr();
	longint UsedFileSize();
	bool NotCached();
	void RdPrefix(bool Chk);
	void WrPrefix();
	void SetEmpty();
	void Create();
	longint NewPage(bool NegL);
	void ReleasePage(longint PosPg);
	void Delete(longint Pos);
	LongStr* Read(WORD StackNr, longint Pos);
	longint Store(LongStrPtr S);
private:
	void RdWr(bool ReadOp, longint Pos, WORD N, void* X);
	void GetMLen();
};
typedef TFile* TFilePtr;

class FileD // ш. 177
{
public:
	FileD();
	FileD* Chain;
	WORD RecLen;
	void* RecPtr;
	longint NRecs;
	bool WasWrRec, WasRdOnly, Eof;
	char Typ;        // 8=Fand 8;6=Fand 16;X= .X; 0=RDB; C=CAT 
	FILE* Handle;
	longint IRec;
	WORD FrstDispl;
	TFile* TF;
	RdbPos ChptPos;     // zero for Rdb and FD translated from string 
	WORD TxtPosUDLI;    // =0 if not present
	FileD* OrigFD;    // like orig. or nil
	BYTE Drive;         // 1=A ,2=B ,else 0
	WORD CatIRec;
	FieldDPtr FldD;
	bool IsParFile, IsJournal, IsHlpFile, typSQLFile, IsSQLFile, IsDynFile;
	FileUseMode UMode;
	LockMode LMode, ExLMode, TaLMode;
	StringList ViewNames;  //after each string BYTE string with user codes 
	XFile* XF;
	KeyDPtr Keys;
	AddD* Add;
	uintptr_t nLDs, LiOfs;
	pstring Name;
	longint UsedFileSize();
	bool IsShared();
	bool NotCached();
	WORD GetNrKeys();
};
typedef FileD* FileDPtr;

struct DBaseFld // ш. 208
{
	pstring Name;
	char Typ = 0;
	longint Displ = 0;
	BYTE Len = 0, Dec = 0;
	BYTE x2[14];
};

struct DBaseHd // ш. 213
{
	BYTE Ver = 0;
	BYTE Date[4] = { 0,0,0,0 };
	longint NRecs = 0;
	WORD HdLen = 0, RecLen = 0;
	DBaseFld Flds[1];
};

struct LinkD // ш. 220
{
	LinkD* Chain;
	WORD IndexRoot;
	BYTE MemberRef; // { 0-no, 1-!, 2-!!(no delete)}
	KeyFldD* Args;
	FileDPtr FromFD, ToFD;
	KeyDPtr ToKey;
	pstring RoleName;
};
typedef LinkD* LinkDPtr;

struct LocVarBlkD // ш. 228
{
	LocVar* Root;
	WORD NParam, Size;
};

struct FuncD // ш. 233
{
	FuncD* Chain;
	char FTyp;
	LocVarBlkD LVB; // {1.LV is result}
	void* Instr; // {InstrPtr}
	pstring Name;
};

struct LocVar // ш. 239
{
	LocVar* Chain;
	bool IsPar;
	char FTyp;
	FileD* FD;
	void* RecPtr;
	pstring Name;
	char Op;
	WORD BPOfs;
	bool IsRetPar;
	FrmlElem* Init;
};

struct RdbD // ш. 243
{
	RdbD* ChainBack = nullptr;
	FileD* FD = nullptr; FileD* HelpFD = nullptr; // { FD=FileDRoot and = Chpt for this RDB }
	LinkD* OldLDRoot = nullptr;
	FuncD* OldFCRoot = nullptr;
	void* Mark2 = nullptr; // { markstore2 at beginning }
	bool Encrypted = false;
	pstring RdbDir, DataDir;
};
typedef RdbD* RdbDPtr;

struct WRectFrml // r251
{
	FrmlPtr C1, R1, C2, R2;
};

class XString // ш. 254
{
public:
	pstring S; // S:string255;
	void Clear(); // index.pas ASM
	void StoreReal(double R, KeyFldD* KF);
	void StoreStr(pstring V, KeyFldD* KF);
	void StoreBool(bool B, KeyFldD* KF);
	void StoreKF(KeyFldD* KF);
	void PackKF(KeyFldD* KF);
	bool PackFrml(FrmlList FL, KeyFldD* KF);
#ifdef FandSQL
	void GetF(WORD Off, WORD Len, bool Descend, void* Buf);
	void GetD(WORD Off, bool Descend, void* R);
	void GetN(WORD Off, WORD Len, bool Descend, void* Buf);
	WORD GetA(WORD Off, WORD Len, bool CompLex, bool Descend, void* Buf);
#endif
private:
	void StoreD(void* R, bool Descend); // index.pas r53 ASM
	void StoreN(void* N, WORD Len, bool Descend); // index.pas r62 ASM
	void StoreF(void* F, WORD Len, bool Descend); // index.pas r68 ASM
	void StoreA(void* A, WORD Len, bool CompLex, bool Descend); // index.pas r76 ASM
};

class XItem // r274
{
public:
	BYTE Nr[3]; // NN  RecNr /on leaf/ or NumberofRecordsBelow
	longint DownPage; // not on leaf
	// M byte  number of equal bytes /not stored bytes/ 
	// Index string  /L=length, A area ptr/
	longint GetN(); // index.pas r129 ASM
	void PutN(longint N); // index.pas r132 ASM
	WORD GetM(WORD O); // index.pas r136 ASM
	void PutM(WORD O, WORD M); // index.pas r139 ASM
	WORD GetL(WORD O); // index.pas r142 ASM
	void PutL(WORD O, WORD L); // index.pas r145 ASM
	XItem* Next(WORD O); // index.pas r148 ASM
	WORD UpdStr(WORD O, pstring* S); // index.pas r152 ASM
};
typedef XItem* XItemPtr;

class XPage // r289
{
public:
	bool IsLeaf;
	longint GreaterPage;  // or free pages chaining
	WORD NItems;
	BYTE A[XPageSize - 4];  // item array
	WORD Off();
	XItem* XI(WORD I);
	uintptr_t EndOff();
	bool Underflow();
	bool Overflow();
	pstring StrI(WORD I);
	longint SumN();
	void Insert(WORD I, void* SS, XItem* XX);
	void InsDownIndex(WORD I, longint Page, XPage* P);
	void Delete(WORD I);
	void AddPage(XPage* P);
	void SplitPage(XPage* P, longint ThisPage);
};
typedef XPage* XPagePtr;

class XKey // r309
{
public:
	XKey* Chain;
	KeyFldD* KFlds;
	bool Intervaltest, Duplic, InWork;
	WORD IndexRoot; BYTE IndexLen;
	longint NR; // {used only by XWKey}
	pstring* Alias;
	XWFile* XF();
	longint NRecs();
	bool Search(XString& XX, bool AfterEqu, longint& RecNr);
	bool SearchIntvl(XString& XX, bool AfterEqu, longint& RecNr);
	longint PathToNr();
	void NrToPath(longint I);
	longint PathToRecNr();
	bool RecNrToPath(XString& XX, longint RecNr);
	bool IncPath(WORD J, longint& Pg);
	longint NrToRecNr(longint I);
	pstring NrToStr(longint I);
	longint RecNrToNr(longint RecNr);
	bool FindNr(XString& X, longint& IndexNr);
	void InsertOnPath(XString& XX, longint RecNr);
	void InsertItem(XString& XX, XPage* P, XPage* UpP, longint Page, WORD I, XItemPtr& X, longint& UpPage);
	void ChainPrevLeaf(XPagePtr P, longint N);
	bool Insert(longint RecNr, bool Try);
	void DeleteOnPath();
	void BalancePages(XPage* P1, XPage* P2, bool& Released);
	void XIDown(XPage* P, XPage* P1, WORD I, longint& Page1);
	bool Delete(longint RecNr);
};

class XWKey : public XKey // r334
{
public:
	void Open(KeyFldD* KF, bool Dupl, bool Intvl);
	void Close();
	void Release();
	void ReleaseTree(longint Page, bool IsClose);
	void OneRecIdx(KeyFldD* KF, longint N);
	void InsertAtNr(longint I, longint RecNr);
	longint InsertGetNr(longint RecNr);
	void DeleteAtNr(longint I);
	void AddToRecNr(longint RecNr, integer Dif);
};

class XWFile // r345
{
public:
	//XWFile();
	WORD UpdLockCnt;
	FILE* Handle;
	longint FreeRoot, MaxPage;
	void Err(WORD N);
	void TestErr();
	longint UsedFileSize();
	bool NotCached();
	void RdPage(XPagePtr P, longint N);
	void WrPage(XPagePtr P, longint N);
	longint NewPage(XPagePtr P);
	void ReleasePage(XPagePtr P, longint N);
};
typedef XWFile* XWFilePtr;

class XFile : public XWFile // r357
{
public:
	// XFile();
	longint NRecs, NRecsAbs; // {FreeRoot..NrKeys read / written by 1 instr.}
	bool NotValid;
	BYTE NrKeys;
	bool NoCreate, FirstDupl;
	void SetEmpty();
	void RdPrefix();
	void WrPrefix();
	void SetNotValid();
};


/// tшнda mб dмdit TObject z Pascalu
/// DOC: https://www.freepascal.org/docs-html/rtl/system/tobject.html
class XScan
{
public:
	FileD* FD;
	KeyD* Key;
	FrmlPtr Bool;
	BYTE Kind;
	longint NRecs, IRec, RecNr;
	bool hasSQLFilter, eof;
	XScan(FileD* aFD, KeyD* aKey, KeyInD* aKIRoot, bool aWithT);
	void Reset(FrmlPtr ABool, bool SQLFilter);
	void ResetSort(KeyFldDPtr aSK, FrmlPtr& BoolZ, LockMode OldMd, bool SQLFilter);
	void SubstWIndex(WKeyDPtr WK);
	void ResetOwner(XString* XX, FrmlPtr aBool);
	void ResetOwnerIndex(LinkDPtr LD, LocVar* LV, FrmlPtr aBool);
#ifdef FandSQL
	void ResetSQLTxt(FrmlPtr Z);
#endif
	void ResetLV(void* aRP);
	void Close();
	void SeekRec(longint I);
	void GetRec();
private:
	KeyInD* KIRoot;
	LocVar* OwnerLV;
	KeyFldD* SK;
	XItem* X;
	XPage* P;
	WORD NOnPg;
	KeyInD* KI;
	longint NOfKI, iOKey;
	bool TempWX, NotFrst, withT;
	void* Strm; // {SQLStreamPtr or LVRecPtr}
	void SeekOnKI(longint I);
	void SeekOnPage(longint Page, WORD I);
	void NextIntvl();
};

struct CompInpD // r402
{
	CompInpD* ChainBack = nullptr;
	CharArr* InpArrPtr = nullptr;
	RdbPos InpRdbPos;
	WORD InpArrLen = 0, CurrPos = 0, OldErrPos = 0;
};

// ********** konstanty ********** // r409
const BYTE _equ = 0x1; const BYTE _lt = 0x2; const BYTE _le = 0x3;
const BYTE _gt = 0x4; const BYTE _ge = 0x5; const BYTE _ne = 0x6;
const BYTE _subrange = 0x7; const BYTE _number = 0x8; const BYTE _assign = 0x9; const BYTE _identifier = 0xA;
const BYTE _addass = 0xB; const BYTE _quotedstr = 0xC;
const BYTE _const = 0x10; //{float/string/boolean};             {0-ary instructions}
const BYTE _field = 0x11; const BYTE _getlocvar = 0x12; // {fieldD}; {BPOfs};
const BYTE _access = 0x13; // {fieldD or nil for exist,newfileD,linkD or nil};
const BYTE _recvarfld = 0x14; // {fieldD,fileD,recptr}
const BYTE _today = 0x18; const BYTE _currtime = 0x19; const BYTE _pi = 0x1A; const BYTE _random = 0x1B;
const BYTE _exitcode = 0x1d; const BYTE _edrecno = 0x1e; const BYTE _getWORDvar = 0x1f; // {n:0..}
const BYTE _memavail = 0x22; const BYTE _maxcol = 0x23; const BYTE _maxrow = 0x24;
const BYTE _getmaxx = 0x25; const BYTE _getmaxy = 0x26;
const BYTE _lastupdate = 0x27; const BYTE _nrecs = 0x28; const BYTE _nrecsabs = 0x29; // {FD}
const BYTE _generation = 0x2a; // {FD}
const BYTE _recno = 0x2b; const BYTE _recnoabs = 0x2c; const BYTE _recnolog = 0x2d;  // {FD,K,Z1,Z2,...}
const BYTE _filesize = 0x2e;  // {txtpath,txtcatirec}
const BYTE _txtpos = 0x2f; const BYTE _cprinter = 0x30; const BYTE _mousex = 0x31; const BYTE _mousey = 0x32;
const BYTE _txtxy = 0x33; const BYTE _indexnrecs = 0x34; const BYTE _owned = 0x35;  // {bool,sum,ld}           // {R}
const BYTE _catfield = 0x36; // {CatIRec,CatFld}
const BYTE _passWORD = 0x37; const BYTE _version = 0x38; const BYTE _username = 0x39; const BYTE _edfield = 0x3a;
const BYTE _accright = 0x3b; const BYTE _readkey = 0x3c; const BYTE _edreckey = 0x3d; const BYTE _edbool = 0x3e;
const BYTE _edfile = 0x3f; const BYTE _edkey = 0x40; const BYTE _clipbd = 0x41; const BYTE _keybuf = 0x42;
const BYTE _keyof = 0x43;  // {LV,KeyD}                                             // {S}
const BYTE _edupdated = 0x44; const BYTE _keypressed = 0x45; const BYTE _escprompt = 0x46;
const BYTE _trust = 0x47; const BYTE _lvdeleted = 0x48;  // {bytestring}_lvdeleted=#$48;   // {LV}                      // {B}
const BYTE _userfunc = 0x49;
const BYTE _isnewrec = 0x4a; const BYTE _mouseevent = 0x4b; const BYTE _ismouse = 0x4c;
const BYTE _testmode = 0x4d;
const BYTE _newfile = 0x60;  // {newfile,newRP};                      // {1-ary instructions}
const BYTE _lneg = 0x61;
const BYTE _inreal = 0x62; const BYTE _instr = 0x63;  // {precision,constlst}; {tilda,constlst};
const BYTE _isdeleted = 0x64; const BYTE _setmybp = 0x65;  // {RecFD}
const BYTE _modulo = 0x66;  // {length,modulo,weight1,...};                          // {B}
const BYTE _getpath = 0x68; const BYTE _upcase = 0x69; const BYTE _lowcase = 0x6A;
const BYTE _leadchar = 0x6B; const BYTE _getenv = 0x6C;
const BYTE _trailchar = 0x6D; const BYTE _strdate = 0x6E;  // {char}  // {maskstring};
const BYTE _nodiakr = 0x6F;  // {S}
const BYTE _char = 0x70; const BYTE _sqlfun = 0x71;  // {SR}
const BYTE _unminus = 0x73; const BYTE _abs = 0x74; const BYTE _int = 0x75; const BYTE _frac = 0x76; const BYTE _sqr = 0x77;
const BYTE _sqrt = 0x78; const BYTE _sin = 0x79; const BYTE _cos = 0x7A; const BYTE _arctan = 0x7b; const BYTE _ln = 0x7c;
const BYTE _exp = 0x7d; const BYTE _typeday = 0x7e; const BYTE _color = 0x7f;
const BYTE _link = 0x90; // {LD}; // {R}
const BYTE _val = 0x91; const BYTE _valdate = 0x92; const BYTE _length = 0x93; const BYTE _linecnt = 0x94; // {maskstring}
const BYTE _diskfree = 0x95; const BYTE _ord = 0x96; const BYTE _eval = 0x97;  // {Typ};  // {RS}
const BYTE _accrecno = 0x98;  // {FD,FldD};    // {R,S,B}
const BYTE _promptyn = 0x99;  // {BS}
const BYTE _conv = 0xa0;   // { used in Prolog}
const BYTE _and = 0xb1; const BYTE _or = 0xb2; const BYTE _limpl = 0xb3; const BYTE _lequ = 0xb4;  // {2-ary instructions}
const BYTE _compreal = 0xb5; const BYTE _compstr = 0xb6;    // {compop,precision}  // {compop,tilda};     // {B}
const BYTE _concat = 0xc0; // {S}
const BYTE _repeatstr = 0xc2; // {SSR}
const BYTE _gettxt = 0xc3; // {txtpath,txtcatirec}       // {SRR}
const BYTE _plus = 0xc4; const BYTE _minus = 0xc5; const BYTE _times = 0xc6; const BYTE _divide = 0xc7;
const BYTE _div = 0xc8; const BYTE _mod = 0xc9; const BYTE _round = 0xca;
const BYTE _addwdays = 0xcb; const BYTE _difwdays = 0xcc; // {typday}
const BYTE _addmonth = 0xcd; const BYTE _difmonth = 0xce; const BYTE _inttsr = 0xcf; // {ptr};
const BYTE _min = 0xd0; const BYTE _max = 0xd1; // {used in Prolog}     // {R}
const BYTE _equmask = 0xd2;   // {BSS}
const BYTE _prompt = 0xd3; // {fieldD};   // {R,S,B}
const BYTE _portin = 0xd4; // {RBR}
const BYTE _cond = 0xf0; // {bool or nil,frml,continue or nil};    // {3-ary instructions}
const BYTE _copy = 0xf1; const BYTE _str = 0xf2; // {S}
const BYTE _selectstr = 0xf3; const BYTE _copyline = 0xf4;  // {SSRR}
const BYTE _pos = 0xf5; const BYTE _replace = 0xf6; // {options}; // {options};   // {RSSR}
const BYTE _mousein = 0xf7;  // {P4};


// ш. 474
static FileD* FileDRoot; // { only current RDB }
static LinkD* LinkDRoot; // { for all RDBs     }
static FuncD* FuncDRoot;
extern FileD* CFile;
static void* CRecPtr;
static KeyD* CViewKey;
static pstring TopRdbDir, TopDataDir;
static pstring CatFDName;
static RdbD* CRdb, TopRdb;
static FileD* CatFD, HelpFD;

// r483
struct structXPath { longint Page; WORD I; } static XPath[10];
static WORD XPathN;
static XWFile XWork;
static TFile TWork;
static longint ClpBdPos = 0;
static bool IsTestRun = false;
static bool IsInstallRun = false;

static FileDPtr Chpt = FileDRoot; // absolute FileDRoot;
static TFilePtr ChptTF;
static FieldDPtr ChptTxtPos;
static FieldDPtr ChptVerif; // { updated record }
static FieldDPtr ChptOldTxt; // { ChptTyp = 'F' : -1 = new unchecked record, else = old declaration }
static FieldDPtr ChptTyp, ChptName, ChptTxt;


// ********** konstanty ********** // r496
const BYTE FloppyDrives = 3;
static bool EscPrompt = false;
static pstring UserName = pstring(20);
static pstring UserPassWORD = pstring(20);
static pstring AccRight;
static bool EdUpdated = false;
static longint EdRecNo = 0;
static pstring EdRecKey = "";
static pstring EdKey = pstring(32);
static bool EdOk = false;
static pstring EdField = pstring(32);
static longint LastTxtPos = 0;
static longint TxtXY = 0;
// { consecutive WORD - sized / for formula access / }
static WORD RprtLine = 0; static WORD RprtPage = 0; static WORD PgeLimit = 0; // {report}
static WORD EdBreak = 0; static WORD EdIRec = 1; // {common - alphabetical order}
static WORD MenuX = 1; static WORD MenuY = 1;
static WORD UserCode = 0;
// **********

static WORD* WordVarArr = &RprtLine;
static FieldDPtr CatRdbName, CatFileName, CatArchiv, CatPathName, CatVolume;
static pstring MountedVol[FloppyDrives] = { pstring(11), pstring(11), pstring(11) };

static pstring SQLDateMask = "DD.MM.YYYY hh:mm:ss";

// ********** COMPARE FUNCTIONS **********
static double Power10[21] = { 1E0, 1E1, 1E2, 1E3, 1E4, 1E5, 1E6, 1E7, 1E8, 1E9, 1E10,
	1E11, 1E12, 1E13, 1E14, 1E15, 1E16, 1E17, 1E18, 1E19, 1E20 };

//void RunErrorM(LockMode Md, WORD N); // r528

integer CompLongStr(LongStrPtr S1, LongStrPtr S2); // r529 ASM
//integer CompStr(pstring S1, pstring S2);
integer CompLongShortStr(LongStrPtr S1, pstring S2); // r551 ASM
integer CompArea(void* A, void* B, integer L); // r575 ASM

//void ResetCompilePars(); // r686
extern BYTE CurrChar; // { Compile }
extern BYTE ForwChar, ExpChar, Lexem;
extern pstring LexWord;

static bool SpecFDNameAllowed, IdxLocVarAllowed, FDLocVarAllowed, IsCompileErr;
static CompInpD* PrevCompInp;						// { saved at "include" }
static CharArrPtr InpArrPtr; static RdbPos InpRdbPos;		// { "  "  }
static WORD InpArrLen, CurrPos, OldErrPos;			// { "  "  }
static SumElPtr FrmlSumEl;				//{ set while reading sum / count argument }
static bool FrstSumVar, FileVarsAllowed;
// FrmlPtr RdFldNameFrml() = FrmlPtr(char& FTyp);
// FrmlPtr RdFunction() = FrmlPtr(char& FTyp);
static FrmlElem* (*RdFldNameFrml)(char&) = nullptr; // ukazatel na funkci
static FrmlElem* (*RdFunction)(char&) = nullptr; // ukazatel na funkci
static void(*ChainSumEl)(); // {set by user}
static BYTE LstCompileVar; // { boundary }

static pstring Switches = "";
static WORD SwitchLevel = 0;

void RunErrorM(LockMode Md, WORD N); // r729
pstring* FieldDMask(FieldDPtr F); // r734 ASM
void* GetRecSpace(); // r739
void* GetRecSpace2(); // r742
WORD CFileRecSize(); // r744
void SetTWorkFlag();  // r746 ASM
bool HasTWorkFlag(); // r752 ASM
void SetUpdFlag(); // r755 ASM
void ClearUpdFlag(); // r758 ASM
bool HasUpdFlag(); // r761 ASM
void* LocVarAd(LocVar* LV); // r766 ASM
bool DeletedFlag(); // r771 ASM
void ClearDeletedFlag(); // r779 ASM
void SetDeletedFlag(); // r785 ASM
integer CompStr(pstring& S1, pstring& S2); // r792 ASM
void CmpLxStr(); // r846 ASM
WORD CompLexLongStr(LongStrPtr S1, LongStrPtr S2); // r854 ASM
WORD CompLexLongShortStr(LongStrPtr S1, pstring& S2); // r863 ASM
WORD CompLexStr(const pstring& S1, const pstring& S2); // r871 ASM
bool EquKFlds(KeyFldDPtr KF1, KeyFldDPtr KF2); // r881
void Code(void* A, WORD L); // r897 ASM
void CodingLongStr(LongStrPtr S);
longint StoreInTWork(LongStrPtr S);
LongStrPtr ReadDelInTWork(longint Pos);
void ForAllFDs(void(*procedure)()); // r935
bool IsActiveRdb(FileDPtr FD);
void ResetCompilePars(); // r953 - posledni fce

// ********** IMPLEMENTATION **********
// od r. 705
// 

//void ClearTWorkFlag(); // r749 ASM
//void TranslateOrd(); // r804 ASM
//WORD TranslateOrdBack(); // r834 ASM
//void XDecode(LongStrPtr S); // r903 ASM
//void DirMinusBackslash(pstring& D);


void ReadRec(longint N);
longint _T(FieldDescr* F);
pstring _ShortS(FieldDPtr F);
void CreateRec(longint N);
void RecallRec(longint RecNr);
bool LinkUpw(LinkDPtr LD, longint& N, bool WithT);
bool LinkLastRec(FileDPtr FD, longint& N, bool WithT);
void IncNRecs(longint N);
void WriteRec(longint N);
void R_(FieldDPtr F, double R);
double _R(FieldDPtr F);
void LongS_(FieldDPtr F, LongStr* S);
void S_(FieldDPtr F, pstring S);
void B_(FieldDPtr F, bool B);
bool TryLMode(LockMode Mode, LockMode& OldMode, WORD Kind);
void OldLMode(LockMode Mode);
LockMode NewLMode(LockMode Mode);
void TestXFExist();
bool TryLockN(longint N, WORD Kind);
void UnLockN(longint N);
LongStr* _LongS(FieldDPtr F);
void ClearRecSpace(void* p);
void ZeroAllFlds();
void DelTFld(FieldDPtr F);
void DelDifTFld(void* Rec, void* CompRec, FieldDPtr F);
bool _B(FieldDPtr F);
void DeleteXRec(longint RecNr, bool DelT);
void OverWrXRec(longint RecNr, void* P2, void* P);
void DelAllDifTFlds(void* Rec, void* CompRec);
void DecNRecs(longint N);
void DeleteRec(longint N);
bool SearchKey(XString& XX, KeyDPtr Key, longint& NN);
longint XNRecs(KeyDPtr K);
void T_(FieldDPtr F, longint Pos);
void AsgnParFldFrml(FileD* FD, FieldDPtr F, FrmlPtr Z, bool Ad);
void PutRec();
void TryInsertAllIndexes(longint RecNr);
void XFNotValid();
void DelTFlds();
void CopyRecWithT(void* p1, void* p2);
void CloseClearHCFile();
void TestCPathError();
void AssignNRecs(bool Add, longint N);
void CExtToT();
void CExtToX();
void CloseGoExit();
void WrPrefixes();
void TestCFileError();
WORD RdPrefix();
bool ChangeLMode(LockMode Mode, WORD Kind, bool RdPref);
void SeekRec(longint N);
void WrPrefix();
void RdPrefixes();



