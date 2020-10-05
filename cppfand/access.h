#pragma once

#include "base.h"
#include "constants.h"
#include "switches.h"
#ifdef FandSQL
#include "channel.h"
#endif


struct FrmlListEl;
class XFile;
class FrmlElem;
class LocVar;
struct FuncD;
class XWKey;
class XWFile;
class XKey;
struct LinkD;
class FileD;
struct RdbD;
class FieldDescr;

// ********** CONST **********
const BYTE LeftJust = 1; // {RightJust=0  coded in M for Typ='N','A'}
const BYTE Ascend = 0; const BYTE Descend = 6; // {used in SortKey}
const WORD XPageSize = 1024; const BYTE XPageOverHead = 7; const BYTE MaxIndexLen = 123; //{ min.4 items };
const BYTE oLeaf = 3; const BYTE oNotLeaf = 7;
const BYTE f_Stored = 1; const BYTE f_Encryp = 2; // {FieldD flags}
const BYTE f_Mask = 4; const BYTE f_Comma = 8; // {FieldD flags}

enum LockMode {
	NullMode = 0, NoExclMode = 1, NoDelMode = 2, NoCrMode = 3,
	RdMode = 4, WrMode = 5, CrMode = 6, DelMode = 7, ExclMode = 8
};
extern pstring LockModeTxt[9];

const WORD MPageSize = 512;
const BYTE XPageShft = 10;
const BYTE MPageShft = 9;

typedef char PwCodeArr[20];

typedef XKey* KeyDPtr;
typedef XKey KeyD;
typedef FuncD* FuncDPtr;
typedef XWKey* WKeyDPtr;


struct FieldListEl : public Chained // r32
{
	FieldDescr* FldD;
};
typedef FieldListEl* FieldList;

struct FrmlListEl : public Chained // ш. 34
{
	FrmlElem* Frml;
};
typedef FrmlListEl* FrmlList;

struct StringListEl : public Chained // ш. 38
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

struct KeyListEl : public Chained // ш. 49
{
	//KeyListEl* Chain;
	KeyD* Key = nullptr;
};
typedef KeyListEl* KeyList;

class FrmlElem // ш. 51
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

class FieldDescr : public Chained // ш. 100
{
public:
	FieldDescr();
	FieldDescr(BYTE* inputStr);
	FieldDescr(const FieldDescr& orig);
	char Typ = 0, FrmlTyp = 0;
	BYTE L = 0, M = 0, NBytes = 0, Flg = 0;
	// case boolean {Stored} of True:(Displ:integer); False:(Frml:FrmlPtr; Name:string[1]{ curr.length });
	integer Displ = 0;
	FrmlElem* Frml = nullptr;
	std::string Name;
};
typedef FieldDescr* FieldDPtr;

class KeyFldD : public Chained // ш. 108
{
public:
	KeyFldD() {};
	KeyFldD(const KeyFldD& orig, bool copyFlds);
	KeyFldD(BYTE* inputStr);
	FieldDescr* FldD = nullptr;
	bool CompLex = false, Descend = false;
};
typedef KeyFldD* KeyFldDPtr;

struct RdbPos // ш. 113
{
	RdbD* R = nullptr;
	WORD IRec = 0;
};

class ChkD : public Chained // ш. 115
{
public:
	ChkD() {};
	ChkD(const ChkD& orig);
	// ChkD* Chain = nullptr;
	FrmlElem* Bool = nullptr;
	pstring* HelpName = nullptr;
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
	FieldDPtr FldD;
	FrmlPtr Frml;
};
typedef ImplD* ImplDPtr;

struct LiRoots
{
	ChkD* Chks; ImplD* Impls;
};
typedef LiRoots* LiRootsPtr;

class AddD // ш. 135
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

class TFile // ш. 147
{
public:
	TFile() {};
	TFile(const TFile& orig);
	FILE* Handle = nullptr;
	longint FreePart = 0;
	bool Reserved = false, CompileProc = false, CompileAll = false;
	WORD IRec = 0;
	longint FreeRoot = 0, MaxPage = 0;
	double TimeStmp = 0.0;
	integer LicenseNr = 0;
	longint MLen = 0;
	PwCodeArr PwCode{ 0 };
	PwCodeArr Pw2Code{ 0 };
	enum eFormat { T00Format, DbtFormat, FptFormat } Format = T00Format;
	WORD BlockSize = 0; // FptFormat
	bool IsWork = false;
	void Err(WORD n, bool ex);
	void TestErr();
	longint UsedFileSize();
	bool NotCached();
	bool Cached();
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

class FileD : public Chained // ш. 177
{
public:
	FileD();
	FileD(const FileD& orig);
	// FileD* Chain = nullptr;
	pstring Name;
	std::string FullName;
	WORD RecLen = 0;
	void* RecPtr = nullptr;
	longint NRecs = 0;
	bool WasWrRec = false, WasRdOnly = false, Eof = false;
	char Typ = 0;        // 8=Fand 8;6=Fand 16;X= .X; 0=RDB; C=CAT 
	FILE* Handle = nullptr;
	longint IRec = 0;
	WORD FrstDispl = 0;
	TFile* TF = nullptr;
	RdbPos ChptPos;     // zero for Rdb and FD translated from string 
	WORD TxtPosUDLI = 0;    // =0 if not present; urcuje zacatek odstavcu #U #D #L #I
	FileD* OrigFD = nullptr;    // like orig. or nil
	BYTE Drive = 0;         // 1=A, 2=B, else 0
	WORD CatIRec = 0;
	std::vector<FieldDescr*> FldD;
	//FieldDescr* FldD = nullptr;
	bool IsParFile = false, IsJournal = false, IsHlpFile = false;
	bool typSQLFile = false, IsSQLFile = false, IsDynFile = false;
	FileUseMode UMode = FileUseMode::Closed;
	LockMode LMode = NullMode, ExLMode = NullMode, TaLMode = NullMode;
	StringListEl* ViewNames = nullptr;  //after each string BYTE string with user codes 
	XFile* XF = nullptr;
	KeyD* Keys = nullptr;
	AddD* Add = nullptr;
	uintptr_t nLDs = 0, LiOfs = 0;
	longint UsedFileSize();
	bool IsShared();
	bool NotCached();
	bool Cached();
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
	BYTE Date[4]{ 0,0,0,0 };
	longint NRecs = 0;
	WORD HdLen = 0, RecLen = 0;
	DBaseFld Flds[1];
};

struct LinkD // ш. 220
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

class LocVarBlkD : public Chained// ш. 228
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
struct FuncD // ш. 233
{
	FuncD* Chain = nullptr;
	char FTyp = '\0';
	LocVarBlkD LVB; // {1.LV is result}
	Instr* pInstr = nullptr; // {InstrPtr}
	pstring Name;
};

class LocVar : public Chained // ш. 239
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

struct RdbD // ш. 243
{
	RdbD* ChainBack = nullptr;
	FileD* FD = nullptr;
	FileD* HelpFD = nullptr; // { FD=FileDRoot and = Chpt for this RDB }
	LinkD* OldLDRoot = nullptr;
	FuncD* OldFCRoot = nullptr;
	void* Mark2 = nullptr; // { markstore2 at beginning }
	bool Encrypted = false;
	std::string RdbDir;
	std::string DataDir;
};
typedef RdbD* RdbDPtr;

struct WRectFrml // r251
{
	FrmlElem* C1 = nullptr;
	FrmlElem* R1 = nullptr;
	FrmlElem* C2 = nullptr;
	FrmlElem* R2 = nullptr;
};

class XString // ш. 254
{
public:
	pstring S; // S:string255;
	//BYTE S[256]{ 0 };
	void Clear(); // index.pas ASM
	void StoreReal(double R, KeyFldD* KF);
	void StoreStr(std::string V, KeyFldD* KF);
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
	XItem(BYTE* data, bool isLeaf);
	BYTE* Nr; // NN  RecNr /on leaf/ or NumberofRecordsBelow
	longint* DownPage; // not on leaf
	// M byte  number of equal bytes /not stored bytes/ 
	// Index string  /L=length, A area ptr/
	BYTE* XPageData;
	longint GetN(); // index.pas r129 ASM
	void PutN(longint N); // index.pas r132 ASM
	WORD GetM(WORD O); // index.pas r136 ASM
	void PutM(WORD O, WORD M); // index.pas r139 ASM
	WORD GetL(WORD O); // index.pas r142 ASM
	void PutL(WORD O, WORD L); // index.pas r145 ASM
	XItem* Next(WORD O, bool isLeaf); // index.pas r148 ASM
	WORD UpdStr(WORD O, pstring* S); // index.pas r152 ASM
	size_t size(bool isLeaf); // vrati delku zaznamu
};
typedef XItem* XItemPtr;

/// implementace XItem pro kratky zaznam
class XItemLeaf
{
public:
	XItemLeaf(BYTE* data);
	XItemLeaf(const XItemLeaf& orig);
	XItemLeaf(unsigned int RecNr, BYTE M, BYTE L, pstring& s); // kompletni 's', zpracuje se jen pozadovana cast
	~XItemLeaf();
	unsigned int RecNr;
	BYTE M;
	BYTE L;
	BYTE* data;
	size_t size();
	size_t dataLen(); // bez 2B L + M
	size_t Serialize(BYTE* buffer, size_t bufferSize);
};

class XPage // r289
{
public:
	XPage() {}
	~XPage();
	bool IsLeaf = false;
	longint GreaterPage = 0;  // or free pages chaining
	WORD NItems = 0;
	BYTE A[XPageSize - 4]{ '\0' };  // item array
	WORD Off();
	XItem* XI(WORD I, bool isLeaf);
	WORD EndOff();
	bool Underflow();
	bool Overflow();
	pstring StrI(WORD I);
	longint SumN();
	void Insert(WORD I, void* SS, XItem** XX);
	void InsertLeaf(unsigned int RecNr, size_t I, pstring& SS);
	void InsDownIndex(WORD I, longint Page, XPage* P);
	void Delete(WORD I);
	void AddPage(XPage* P);
	void SplitPage(XPage* P, longint ThisPage);
	void Clean();
	size_t ItemsSize();
	void GenArrayFromVectorItems();
private:
	XItem* _xItem = nullptr;
	void genItems();
	std::vector<XItemLeaf*>::iterator _addToItems(XItemLeaf* xi, size_t pos);
	std::vector<XItemLeaf*> _leafItems;
	bool _cutLeafItem(size_t iIndex, BYTE length); // zkrati polozku o X Bytu, zaktualizuje M i L
	bool _enhLeafItem(size_t iIndex, BYTE length); // prodlouzi polozku o X Bytu z predchozi polozky, zaktualizuje M i L
};
typedef XPage* XPagePtr;

class XKey // r309
{
public:
	XKey();
	XKey(const XKey& orig, bool copyFlds);
	XKey(BYTE* inputStr);
	XKey* Chain = nullptr;
	KeyFldD* KFlds = nullptr;
	bool Intervaltest = false, Duplic = false, InWork = false;
	WORD IndexRoot = 0; BYTE IndexLen = 0;
	longint NR = 0; // {used only by XWKey}
	pstring* Alias = nullptr;
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
	void InsertItem(XString& XX, XPage* P, XPage* UpP, longint Page, WORD I, XItem** X, longint& UpPage);
	void InsertLeafItem(XString& XX, XPage* P, XPage* UpP, longint Page, WORD I, int RecNr, longint& UpPage);
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
	WORD UpdLockCnt = 0;
	FILE* Handle = nullptr;
	longint FreeRoot = 0, MaxPage = 0;
	void Err(WORD N);
	void TestErr();
	longint UsedFileSize();
	bool NotCached();
	void RdPage(XPage* P, longint N);
	void WrPage(XPage* P, longint N);
	longint NewPage(XPage* P);
	void ReleasePage(XPage* P, longint N);
};
typedef XWFile* XWFilePtr;

class XFile : public XWFile // r357
{
public:
	XFile() {};
	XFile(const XFile& orig);
	longint NRecs = 0, NRecsAbs = 0; // {FreeRoot..NrKeys read / written by 1 instr.}
	bool NotValid = false;
	BYTE NrKeys = 0;
	bool NoCreate = false, FirstDupl = false;
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
	FileD* FD = nullptr;
	KeyD* Key = nullptr;
	FrmlElem* Bool = nullptr;
	BYTE Kind = 0;
	longint NRecs = 0, IRec = 0, RecNr = 0;
	bool hasSQLFilter = false, eof = false;
	XScan(FileD* aFD, KeyD* aKey, KeyInD* aKIRoot, bool aWithT);
	void Reset(FrmlElem* ABool, bool SQLFilter);
	void ResetSort(KeyFldD* aSK, FrmlPtr& BoolZ, LockMode OldMd, bool SQLFilter);
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
	KeyInD* KIRoot = nullptr;
	LocVar* OwnerLV = nullptr;
	KeyFldD* SK = nullptr;
	XItem* X = nullptr;
	XPage* P = nullptr;
	WORD NOnPg = 0;
	KeyInD* KI = nullptr;
	longint NOfKI = 0, iOKey = 0;
	bool TempWX = false, NotFrst = false, withT = false;
	void* Strm = nullptr; // {SQLStreamPtr or LVRecPtr}
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
const BYTE _password = 0x37; const BYTE _version = 0x38; const BYTE _username = 0x39; const BYTE _edfield = 0x3a;
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
const BYTE _trailchar = 0x6D; const BYTE _strdate1 = 0x6E;  // {char}  // {maskstring};
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
extern FileD* FileDRoot; // { only current RDB }
extern LinkD* LinkDRoot; // { for all RDBs     }
extern FuncD* FuncDRoot;
extern FileD* CFile;
extern void* CRecPtr;
extern KeyD* CViewKey;
extern pstring TopRdbDir, TopDataDir;
extern pstring CatFDName;
extern RdbD* CRdb, TopRdb;
extern FileD* CatFD, HelpFD;

// r483
struct structXPath { longint Page; WORD I; };
extern structXPath XPath[10];
extern WORD XPathN;
extern XWFile XWork;
extern TFile TWork;
extern longint ClpBdPos;
extern bool IsTestRun;
extern bool IsInstallRun;

extern FileD* Chpt; // absolute FileDRoot;
extern TFile* ChptTF;
extern FieldDPtr ChptTxtPos;
extern FieldDPtr ChptVerif; // { updated record }
extern FieldDPtr ChptOldTxt; // { ChptTyp = 'F' : -1 = new unchecked record, else = old declaration }
extern FieldDPtr ChptTyp, ChptName, ChptTxt;


// ********** konstanty ********** // r496
extern bool EscPrompt;
extern pstring UserName;
extern pstring UserPassWORD;
extern pstring AccRight;
extern bool EdUpdated;
extern longint EdRecNo;
extern pstring EdRecKey;
extern pstring EdKey;
extern bool EdOk;
extern pstring EdField;
extern longint LastTxtPos;
extern longint TxtXY;
// { consecutive WORD - sized / for formula access / }
extern WORD RprtLine; extern WORD RprtPage; extern WORD PgeLimit; // {report}
extern WORD EdBreak; extern WORD EdIRec; // {common - alphabetical order}
extern WORD MenuX; extern WORD MenuY;
extern WORD UserCode;
// **********

// extern WORD* WordVarArr;
extern FieldDPtr CatRdbName, CatFileName, CatArchiv, CatPathName, CatVolume;
extern pstring MountedVol[FloppyDrives];

extern pstring SQLDateMask;

// ********** COMPARE FUNCTIONS **********
extern double Power10[21];

//void RunErrorM(LockMode Md, WORD N); // r528

integer CompLongStr(LongStr* S1, LongStr* S2); // r529 ASM
//integer CompStr(pstring S1, pstring S2);
integer CompLongShortStr(LongStr* S1, pstring* S2); // r551 ASM
integer CompArea(void* A, void* B, integer L); // r575 ASM

//void ResetCompilePars(); // r686
extern BYTE CurrChar; // { Compile }
extern BYTE ForwChar, ExpChar, Lexem;
extern pstring LexWord;

extern bool SpecFDNameAllowed, IdxLocVarAllowed, FDLocVarAllowed, IsCompileErr;
extern CompInpD* PrevCompInp;						// { saved at "include" }
extern BYTE* InpArrPtr; extern RdbPos InpRdbPos;		// { "  "  }
extern WORD InpArrLen;
extern WORD CurrPos;
extern WORD OldErrPos;			// { "  "  }
extern SumElPtr FrmlSumEl;				//{ set while reading sum / count argument }
extern bool FrstSumVar, FileVarsAllowed;
// FrmlPtr RdFldNameFrml() = FrmlPtr(char& FTyp);
// FrmlPtr RdFunction() = FrmlPtr(char& FTyp);
extern FrmlElem* (*RdFldNameFrml)(char&); // ukazatel na funkci
extern FrmlElem* (*RdFunction)(char&); // ukazatel na funkci
extern void(*ChainSumEl)(); // {set by user}
extern BYTE LstCompileVar; // { boundary }

extern pstring Switches;
extern WORD SwitchLevel;

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
bool EquKFlds(KeyFldDPtr KF1, KeyFldDPtr KF2); // r881
void Code(std::string& data);
void Code(void* A, WORD L); // r897 ASM
void CodingLongStr(LongStrPtr S);
longint StoreInTWork(LongStr* S);
LongStrPtr ReadDelInTWork(longint Pos);
void ForAllFDs(void(*procedure)()); // r935
bool IsActiveRdb(FileDPtr FD);
void ResetCompilePars(); // r953 - posledni fce

// ********** IMPLEMENTATION **********
// od r. 705
// 

//void ClearTWorkFlag(); // r749 ASM
std::string TranslateOrd(std::string text); // r804 ASM
//WORD TranslateOrdBack(); // r834 ASM
//void XDecode(LongStrPtr S); // r903 ASM
//void DirMinusBackslash(pstring& D);


// * CTENI A ZAPISOVANI SOUBORU *
//void ReadRec(longint N);
void ReadRec(FileD* file, longint N, void* record);
void WriteRec(longint N);
void WriteRec(FileD* file, longint N, void* record);

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
void DelTFld(FieldDPtr F);
void DelDifTFld(void* Rec, void* CompRec, FieldDPtr F);
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
