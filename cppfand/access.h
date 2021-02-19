#pragma once

#include "base.h"
#include "constants.h"
#include "Rdb.h"
#include "switches.h"
#include "XString.h"
#include "XWFile.h"
#include "XWKey.h"
#ifdef FandSQL
#include "channel.h"
#endif

class FileD;
class KeyFldD;
class TFile;
class XFile;
struct FrmlListEl;
class FrmlElem;
class LocVar;
struct FuncD;
class XWKey;
class XWFile;
class XKey;
struct LinkD;
class FieldDescr;

// ********** CONST **********
const BYTE LeftJust = 1; // {RightJust=0  coded in M for Typ='N','A'}
const BYTE Ascend = 0; const BYTE Descend = 6; // {used in SortKey}
const BYTE f_Stored = 1; const BYTE f_Encryp = 2; // {FieldD flags}
const BYTE f_Mask = 4; const BYTE f_Comma = 8; // {FieldD flags}

const WORD MPageSize = 512;
const BYTE XPageShft = 10;
const BYTE MPageShft = 9;

typedef FuncD* FuncDPtr;
typedef XWKey* WKeyDPtr;


struct FieldListEl : public Chained // r32
{
	FieldDescr* FldD;
};
typedef FieldListEl* FieldList;

struct FrmlListEl : public Chained // ø. 34
{
	FrmlElem* Frml;
};
typedef FrmlListEl* FrmlList;

struct StringListEl : public Chained // ø. 38
{
	std::string S;
};
typedef StringListEl* StringList;

struct FloatPtrListEl // r42
{
	FloatPtrListEl* Chain;
	double* RPtr;
};
typedef FloatPtrListEl* FloatPtrList;

struct KeyListEl : public Chained // ø. 49
{
	//KeyListEl* Chain;
	KeyD* Key = nullptr;
};
typedef KeyListEl* KeyList;

class FrmlElem // ø. 51
{
public:
	FrmlElem(BYTE Op, size_t buff_size) { this->Op = Op; /*buffer = new BYTE[buff_size]{ 0 };*/ }
	//~FrmlElem() { delete[] buffer; }
	BYTE Op = 0;
	//BYTE* buffer = nullptr;
};
typedef FrmlElem* FrmlPtr;

struct KeyInD : public Chained // r89
{
	FrmlListEl* FL1 = nullptr;
	FrmlListEl* FL2 = nullptr;
	longint XNrBeg = 0, N = 0;
	std::string X1;
	std::string X2;
};

struct SumElem // r95
{
	SumElem* Chain = nullptr;
	char Op = '\0';
	double R = 0.0;
	FrmlElem* Frml = nullptr;
};
typedef SumElem* SumElPtr;

struct structXPath { longint Page; WORD I; };

class ChkD : public Chained // ø. 115
{
public:
	ChkD() {};
	ChkD(const ChkD& orig);
	// ChkD* Chain = nullptr;
	FrmlElem* Bool = nullptr;
	std::string* HelpName = nullptr;
	FrmlElem* TxtZ = nullptr;
	bool Warning = false;
};
typedef ChkD* ChkDPtr;

struct DepD : Chained // r122
{
	//DepD* Chain; 
	FrmlPtr Bool, Frml;
};
typedef DepD* DepDPtr;

struct ImplD : public Chained
{
	//ImplD* Chain; 
	FieldDescr* FldD;
	FrmlPtr Frml;
};
typedef ImplD* ImplDPtr;

struct LiRoots
{
	ChkD* Chks; ImplD* Impls;
};
typedef LiRoots* LiRootsPtr;

class AddD // ø. 135
{
public:
	AddD() {};
	AddD(const AddD& orig);
	AddD* Chain = nullptr;
	FieldDescr* Field = nullptr;
	FileD* File2 = nullptr;
	LinkD* LD = nullptr;
	BYTE Create = 0; // { 0-no, 1-!, 2-!! }
	FrmlElem* Frml = nullptr;
	bool Assign = false;
	FrmlElem* Bool = nullptr;
	ChkD* Chk = nullptr;
};
typedef AddD* AddDPtr;

struct DBaseFld // ø. 208
{
	pstring Name;
	char Typ = 0;
	longint Displ = 0;
	BYTE Len = 0, Dec = 0;
	BYTE x2[14];
};

struct DBaseHd // ø. 213
{
	BYTE Ver = 0;
	BYTE Date[4]{ 0,0,0,0 };
	longint NRecs = 0;
	WORD HdLen = 0, RecLen = 0;
	DBaseFld Flds[1];
};

struct LinkD // ø. 220
{
	LinkD* Chain = nullptr;
	WORD IndexRoot = 0;
	BYTE MemberRef = 0; // { 0-no, 1-!, 2-!!(no delete)}
	KeyFldD* Args = nullptr;
	FileD* FromFD = nullptr; FileD* ToFD = nullptr;
	KeyD* ToKey = nullptr;
	pstring RoleName;
};
typedef LinkD* LinkDPtr;

class LocVarBlkD : public Chained// ø. 228
{
public:
	LocVarBlkD() {  }
	//~LocVarBlkD() {
	//	for (size_t i = 0; i < vLocVar.size(); i ++) { delete vLocVar[i]; }
	//}
	LocVar* GetRoot();
	LocVar* FindByName(std::string Name);
	std::string FceName;
	std::vector<LocVar*> vLocVar;
	//LocVar* Root = nullptr;
	WORD NParam = 0;
	WORD Size = 0;
};

class Instr;
struct FuncD // ø. 233
{
	FuncD* Chain = nullptr;
	char FTyp = '\0';
	LocVarBlkD LVB; // {1.LV is result}
	Instr* pInstr = nullptr; // {InstrPtr}
	pstring Name;
};

class LocVar : public Chained // ø. 239
{
public:
	LocVar() = default;
	LocVar(std::string Name) { this->Name = Name; }
	bool IsPar = false; // urcuje, zda se jedna o vstupni parametr
	bool IsRetPar = false; // urcuje, zda jde o parametr predavany odkazem
	bool IsRetValue = false; // pridano navic - urcuje navratovou hodnotu funkce
	char FTyp = '\0';
	FileD* FD = nullptr;
	void* RecPtr = nullptr;
	std::string Name;
	char Op = '\0';
	WORD BPOfs = 0;
	FrmlElem* Init = nullptr;

	bool B = false;
	double R = 0.0;
	std::string S;
	WORD orig_S_length = 0;
};

struct WRectFrml // r251
{
	FrmlElem* C1 = nullptr;
	FrmlElem* R1 = nullptr;
	FrmlElem* C2 = nullptr;
	FrmlElem* R2 = nullptr;
};

struct CompInpD // r402
{
	CompInpD* ChainBack = nullptr;
	CharArr* InpArrPtr = nullptr;
	RdbPos InpRdbPos;
	WORD InpArrLen = 0, CurrPos = 0, OldErrPos = 0;
};

//void RunErrorM(LockMode Md, WORD N); // r528

integer CompLongStr(LongStr* S1, LongStr* S2); // r529 ASM
integer CompLongShortStr(LongStr* S1, pstring* S2); // r551 ASM
integer CompArea(void* A, void* B, integer L); // r575 ASM

void RunErrorM(LockMode Md, WORD N); // r729
pstring FieldDMask(FieldDescr* F); // r734 ASM
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
WORD CompLexLongStr(LongStrPtr S1, LongStrPtr S2); // r854 ASM
WORD CompLexLongShortStr(LongStrPtr S1, pstring& S2); // r863 ASM
WORD CompLexStr(pstring& S1, pstring& S2); // r871 ASM
WORD CompLexStrings(const std::string& S1, const std::string& S2);
bool EquKFlds(KeyFldD* KF1, KeyFldD* KF2); // r881
void Code(std::string& data);
void Code(void* A, WORD L); // r897 ASM
void CodingLongStr(LongStrPtr S);
longint StoreInTWork(LongStr* S);
LongStrPtr ReadDelInTWork(longint Pos);
void ForAllFDs(void(*procedure)()); // r935
bool IsActiveRdb(FileD* FD);
void ResetCompilePars(); // r953 - posledni fce

// ********** IMPLEMENTATION **********
// od r. 705
// 

//void ClearTWorkFlag(); // r749 ASM
std::string TranslateOrd(std::string text); // r804 ASM
//WORD TranslateOrdBack(); // r834 ASM
//void XDecode(LongStrPtr S); // r903 ASM
//void DirMinusBackslash(pstring& D);

// * NACITANI ZE SOUBORU / Z FRMLELEM *
bool _B(FieldDescr* F);
double _R(FieldDescr* F);
pstring _ShortS(FieldDescr* F);
std::string _StdS(FieldDescr* F);
LongStr* _LongS(FieldDescr* F);
longint _T(FieldDescr* F);
longint _T(FieldDescr* F, unsigned char* data, char Typ);
// * UKLADANI DO SOUBORU * / DO FRMLELEM *
void B_(FieldDescr* F, bool B);
void R_(FieldDescr* F, double R);
void S_(FieldDescr* F, std::string S, void* record = nullptr);
void LongS_(FieldDescr* F, LongStr* S);
void T_(FieldDescr* F, longint Pos);

void CreateRec(longint N);
void RecallRec(longint RecNr);
bool LinkUpw(LinkDPtr LD, longint& N, bool WithT);
bool LinkLastRec(FileD* FD, longint& N, bool WithT);
void IncNRecs(longint N);
bool TryLMode(LockMode Mode, LockMode& OldMode, WORD Kind);
void OldLMode(LockMode Mode);
LockMode NewLMode(LockMode Mode);
void TestXFExist();
bool TryLockN(longint N, WORD Kind);
void UnLockN(longint N);
void ClearRecSpace(void* p);
void ZeroAllFlds();
void DelTFld(FieldDescr* F);
void DelDifTFld(void* Rec, void* CompRec, FieldDescr* F);
void DeleteXRec(longint RecNr, bool DelT);
void OverWrXRec(longint RecNr, void* P2, void* P);
void DelAllDifTFlds(void* Rec, void* CompRec);
void DecNRecs(longint N);
void DeleteRec(longint N);
bool SearchKey(XString& XX, KeyDPtr Key, longint& NN);
longint XNRecs(KeyDPtr K);
void AsgnParFldFrml(FileD* FD, FieldDescr* F, FrmlElem* Z, bool Ad);
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

void FixFromReal(double r, void* FixNo, WORD FLen);