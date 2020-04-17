#pragma once

#include <string>
#include "constants.h"
#include "fanddml.h"
#include "runfand.h"

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
const WORD XPageSize = 1024;
const BYTE f_Stored = 1; const BYTE f_Encryp = 2; // {FieldD flags}
const BYTE f_Mask = 4; const BYTE f_Comma = 8; // {FieldD flags}

enum FileUseMode { Closed, RdOnly, RdShared, Shared, Exclusive };
enum LockMode { NullMode, NoExclMode, NoDelMode, NoCrMode, RdMode, WrMode, CrMode, DelMode, ExclMode };

typedef char PwCodeArr[20];

typedef XKey* KeyDPtr;
typedef XKey KeyD;
typedef FuncD* FuncDPtr;
typedef XWKey* WKeyDPtr;


struct FrmlListEl // ø. 34
{
	FrmlListEl* Chain;
	FrmlElem* Frml;
};
typedef FrmlListEl* FrmlList;

struct StringListEl // ø. 38
{
	StringListEl* Chain;
	string S;
};
typedef StringListEl* StringList;

struct KeyListEl // ø. 49
{
	KeyListEl* Chain;
	KeyDPtr Key;
};
typedef KeyListEl* KeyList;


struct FrmlElem // ø. 51
{
	char Op;
	FrmlElem* P1; FrmlElem* P2; FrmlElem* P3; FrmlElem* P4; FrmlElem* P5; FrmlElem* P6; // 0
	char Delim; // 0
	BYTE N01, N02, N03, N04, N11, N12, N13, N14, N21, N22, N23, N24, N31; // 1
	BYTE W01, W02, W11, W12, W21, W22; // 1
	float R; // 2
	string S; // 4
	bool B; // 5
	FrmlElem* PP1; pstring Mask; // 6
	FieldDescr* Field; // 7 {_field}
	FrmlElem* P011; FileD* File2; LinkD* LD; // 7  {LD=nil for param} {_access} {LD=RecPtr} {_recvarfld}
	FrmlElem* Frml; FileD* NewFile; void* NewRP; // 8 {_newfile}
	FileD* FD; // 9 {_lastupdate, _generation}
	WORD CatIRec; FileD* CatFld; // 10 {_catfield}
	FrmlElem* PPP1; FrmlElem* PP2; FileD* FldD; // 11 {_prompt}
	FrmlElem* PPPP1; FrmlElem* PPP2; FrmlElem* PP3; pstring Options; // 12 {_pos,_replace}
	FileD* FFD; KeyD* Key; FrmlElem* Arg[2]; // 13 {_recno/typ='R' or'S'/,_recnoabs,_recnolog}
	FrmlElem* PPPPP1; FileD* RecFD; FileD* RecFldD; // 14 {_accrecno,_isdeleted}
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


struct FieldDescr // ø. 100
{
	FieldDescr* Chain;
	char Typ, FrmlTyp;
	BYTE L, M, NBytes, Flg;
	// case boolean {Stored} of True:(Displ:integer); False:(Frml:FrmlPtr; Name:string[1]{ curr.length });
	integer Displ;
	FrmlPtr Frml;
	string Name;
};
typedef FieldDescr* FieldDPtr;

struct KeyFldD // ø. 108
{
	KeyFldD* Chain;
	FieldDPtr FldD;
	bool CompLex, Descend;
};
typedef KeyFldD* KeyFldDPtr;

struct RdbPos // ø. 113
{
	RdbD* R;
	WORD IRec;
};

struct ChkD // ø. 115
{
	ChkD* Chain;
	FrmlPtr Bool;
	pstring* HelpName;
	FrmlPtr TxtZ;
	bool Warning;
};
typedef ChkD* ChkDPtr;

struct AddD // ø. 135
{
	AddD* Chain;
	FieldDPtr Field;
	FileD* File2;
	LinkD* LD;
	BYTE Create; // {0- no,1-!,2-!!}
	FrmlPtr Frml;
	bool Assign;
	FrmlPtr Bool;
	ChkDPtr Chk;
};
typedef AddD* AddDPtr;

class TFile // ø. 147
{
public:
	WORD Handle;
	longint FreePart;
	bool Reserved, CompileProc, CompileAll;
	WORD IRec;
	longint FreeRoot, MaxPage;
	float TimeStmp;
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
	LongStrPtr Read(WORD StackNr, longint Pos);
	longint Store(LongStrPtr S);
};
typedef TFile* TFilePtr;

class FileD // ø. 177
{
public:
	FileD();
	FileD* Chain;
	WORD RecLen;
	void* RecPtr;
	longint NRecs;
	bool WasWrRec, WasRdOnly, Eof;
	char Typ;        // 8=Fand 8;6=Fand 16;X= .X; 0=RDB; C=CAT 
	WORD Handle;
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
	WORD nLDs, LiOfs;
	pstring Name;
	longint UsedFileSize();
	bool IsShared();
	bool NotCached();
	WORD GetNrKeys();
};
typedef FileD* FileDPtr;

struct DBaseFld // ø. 208
{
	string Name;
	char Typ;
	longint Displ;
	BYTE Len, Dec;
	BYTE x2[14];
};

struct DBaseHd // ø. 213
{
	BYTE Ver;
	BYTE Date[4];
	longint NRecs;
	WORD HdLen, RecLen;
	DBaseFld Flds[1];
};

struct LinkD // ø. 220
{
	LinkD* Chain;
	WORD IndexRoot;
	BYTE MemberRef; // { 0-no, 1-!, 2-!!(no delete)}
	KeyFldD* Args;
	FileDPtr FromFD, ToFD;
	KeyDPtr ToKey;
	string RoleName;
};
typedef LinkD* LinkDPtr;

struct LocVarBlkD // ø. 228
{
	LocVar* Root;
	WORD NParam, Size;
};

struct FuncD // ø. 233
{
	FuncD* Chain;
	char FTyp;
	LocVarBlkD LVB; // {1.LV is result}
	void* Instr; // {InstrPtr}
	string Name;
};

struct LocVar // ø. 239
{
	LocVar* Chain;
	bool IsPar;
	char FTyp;
	FileD* FD;
	void* RecPtr;
	string Name;
	char Op;
	WORD BPOfs;
	bool IsRetPar;
	FrmlElem* Init;
};

struct RdbD // ø. 243
{
	RdbD* ChainBack;
	FileDPtr FD, HelpFD; // { FD=FileDRoot and =Chpt for this RDB }
	LinkDPtr OldLDRoot;
	FuncDPtr OldFCRoot;
	void* Mark2; // { markstore2 at beginning }
	bool Encrypted;
	pstring RdbDir, DataDir;
};
typedef RdbD* RdbDPtr;

class XString // ø. 254
{
	string S; // S:string255;
	void Clear();
	void StoreReal(double R, KeyFldD* KF);
	void StoreStr(string V, KeyFldD* KF);
	void StoreBool(bool B, KeyFldD* KF);
	void StoreKF(KeyFldD* KF);
	void PackKF(KeyFldD* KF);
	bool PackFrml(FrmlList FL, KeyFldD* KF);
};

class XItem // ø. 274
{
public:
	BYTE Nr[3]; // NN  RecNr /on leaf/ or NumberofRecordsBelow
	longint DownPage; // not on leaf
	// M byte  number of equal bytes /not stored bytes/ 
	// Index string  /L=length, A area ptr/
	longint GetN();
	void PutN(longint N);
	WORD GetM(WORD O);
	void PutM(WORD O, WORD M);
	WORD GetL(WORD O);
	void PutL(WORD O, WORD L);
	XItem* Next(WORD O);
	WORD UpdStr(WORD O, pstring* S);

};
typedef XItem* XItemPtr;

class XPage // ø. 289
{
	bool IsLeaf;
	longint GreaterPage;  // or free pages chaining
	WORD NItems;
	BYTE A[XPageSize - 4];  // item array
	WORD Off();
	XItem* XI(WORD I);
	WORD EndOff();
	bool Underflow();
	bool Overflow();
	string StrI(WORD I);
	longint SumN();
	void Insert(WORD I, void* SS, XItem* XX);
	void InsDownIndex(WORD I, longint Page, XPage* P);
	void Delete(WORD I);
	void AddPage(XPage* P);
	void SplitPage(XPage* P, longint ThisPage);
};
typedef XPage* XPagePtr;

class XKey // ø. 309
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
	longint NrToRecNr(longint I);
	string NrToStr(longint I);
	longint RecNrToNr(longint RecNr);
	bool FindNr(XString& X, longint& IndexNr);
	void InsertOnPath(XString& XX, longint RecNr);
	bool Insert(longint RecNr, bool Try);
	void DeleteOnPath();
	bool Delete(longint RecNr);
};

class XWKey : public XKey // ø. 334
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

class XWFile // ø. 345
{
public:
	XWFile();
	WORD UpdLockCnt, Handle;
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

class XFile : public XWFile // ø. 357
{
public:
	XFile();
	longint NRecs, NRecsAbs; // {FreeRoot..NrKeys read / written by 1 instr.}
	bool NotValid;
	BYTE NrKeys;
	bool NoCreate, FirstDupl;
	void SetEmpty();
	void RdPrefix();
	void WrPrefix();
	void SetNotValid();
};

// ø. 474
FileDPtr FileDRoot; // { only current RDB }
LinkDPtr LinkDRoot; // { for all RDBs     }
FuncDPtr FuncDRoot;
FileDPtr CFile;
void* CRecPtr;
KeyDPtr CViewKey;
pstring TopRdbDir, TopDataDir;
pstring CatFDName;
RdbDPtr CRdb, TopRdb;
FileDPtr CatFD, HelpFD;

// ø. 483
struct { longint Page; WORD I; } XPath[10];
WORD XPathN;
XWFile XWork;
TFile TWork;
const longint ClpBdPos = 0;
bool IsTestRun = false;
bool IsInstallRun = false;

// ø. 497
const BYTE FloppyDrives = 3;

// ø. 517
FieldDPtr CatRdbName, CatFileName, CatArchiv, CatPathName, CatVolume;
pstring MountedVol[FloppyDrives];

void* GetRecSpace(); // ø. 739

