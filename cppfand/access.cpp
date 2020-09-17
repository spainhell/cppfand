#include "access.h"

#include "../pascal/random.h"
#include "../pascal/real48.h"
#include "../pascal/asm.h"
#include "compile.h"
#include "legacy.h"
#include "oaccess.h"
#include "obaseww.h"
#include "olongstr.h"
#include "runfrml.h"
#include "sort.h"

WORD randIndex = 0;

FileD* CFile;
FieldDPtr CatRdbName, CatFileName, CatArchiv, CatPathName, CatVolume;
FileD* FileDRoot; // { only current RDB }
LinkD* LinkDRoot; // { for all RDBs     }
FuncD* FuncDRoot;
void* CRecPtr;
KeyD* CViewKey;
pstring TopRdbDir, TopDataDir;
pstring CatFDName;
RdbD* CRdb, TopRdb;
FileD* CatFD, HelpFD;
WORD InpArrLen, CurrPos, OldErrPos;

pstring LockModeTxt[9] = { "NULL", "NOEXCL","NODEL","NOCR","RD","WR","CR","DEL","EXCL" };

structXPath XPath[10];
WORD XPathN;
XWFile XWork;
TFile TWork;
longint ClpBdPos = 0;
bool IsTestRun = false;
bool IsInstallRun = false;
FileD* Chpt = FileDRoot; // absolute FileDRoot;
TFilePtr ChptTF;
FieldDPtr ChptTxtPos;
FieldDPtr ChptVerif; // { updated record }
FieldDPtr ChptOldTxt; // { ChptTyp = 'F' : -1 = new unchecked record, else = old declaration }
FieldDPtr ChptTyp, ChptName, ChptTxt;
bool EscPrompt = false;
pstring UserName = pstring(20);
pstring UserPassWORD = pstring(20);
pstring AccRight;
bool EdUpdated = false;
longint EdRecNo = 0;
pstring EdRecKey = "";
pstring EdKey = pstring(32);
bool EdOk = false;
pstring EdField = pstring(32);
longint LastTxtPos = 0;
longint TxtXY = 0;
// { consecutive WORD - sized / for formula access / }
WORD RprtLine = 0; WORD RprtPage = 0; WORD PgeLimit = 0; // {report}
WORD EdBreak = 0; WORD EdIRec = 1; // {common - alphabetical order}
WORD MenuX = 1; WORD MenuY = 1;
WORD UserCode = 0;
WORD* WordVarArr = &RprtLine;
pstring MountedVol[FloppyDrives] = { pstring(11), pstring(11), pstring(11) };
pstring SQLDateMask = "DD.MM.YYYY hh:mm:ss";
double Power10[21] = { 1E0, 1E1, 1E2, 1E3, 1E4, 1E5, 1E6, 1E7, 1E8, 1E9, 1E10,
	1E11, 1E12, 1E13, 1E14, 1E15, 1E16, 1E17, 1E18, 1E19, 1E20 };
bool SpecFDNameAllowed, IdxLocVarAllowed, FDLocVarAllowed, IsCompileErr;
CompInpD* PrevCompInp;						// { saved at "include" }
BYTE* InpArrPtr; RdbPos InpRdbPos;		// { "  "  }
SumElPtr FrmlSumEl;				//{ set while reading sum / count argument }
bool FrstSumVar, FileVarsAllowed;
// FrmlPtr RdFldNameFrml() = FrmlPtr(char& FTyp);
// FrmlPtr RdFunction() = FrmlPtr(char& FTyp);
FrmlElem* (*RdFldNameFrml)(char&) = nullptr; // ukazatel na funkci
FrmlElem* (*RdFunction)(char&) = nullptr; // ukazatel na funkci
void(*ChainSumEl)(); // {set by user}
BYTE LstCompileVar; // { boundary }

pstring Switches = "";
WORD SwitchLevel = 0;


integer CompLongStr(LongStr* S1, LongStr* S2)
{
	integer result = 0;
	if (S1->LL != S2->LL) {
		if (S1->LL < S2->LL) return _lt;
		else return _gt;
	}
	if (S2->LL == 0) return _equ;
	for (size_t i = 0; i < S2->LL; i++)
	{
		if (S1->A[i] == S2->A[i]) continue;
		if (S1->A[i] < S2->A[i]) return _lt;
		return _gt;
	}
	return _equ;
}

integer CompLongShortStr(LongStr* S1, pstring* S2)
{
	integer result = 0;
	if (S1->LL != (*S2)[0]) {
		if (S1->LL < (*S2)[0]) result = 2;
		else result = 4;
	}
	if ((*S2)[0] == 0) return result;
	for (size_t i = 0; i < (*S2)[0]; i++)
	{
		if (S1->A[i] == (*S2)[i + 1]) continue;
		if (S1->A[i] < (*S2)[i + 1]) return 2;
		return 4;
	}
	return 0;
}

integer CompArea(void* A, void* B, integer L)
{
	auto result = memcmp(A, B, L);

	// 1 jsou si rovny
	// 2 A<B
	// 4 A>B

	if (result == 0) return _equ;
	if (result < 0) return 2;
	return 4;
}

#ifdef FandNetV
void ModeLockBnds(LockMode Mode, longint& Pos, WORD& Len)
{
	longint n = 0;
	switch (Mode) {       /* hi=how much BYTEs, low= first BYTE */
	case NoExclMode: n = 0x00010000 + LANNode; break;
	case NoDelMode: n = 0x00010100 + LANNode; break;
	case NoCrMode: n = 0x00010200 + LANNode; break;
	case RdMode: n = 0x00010300 + LANNode; break;
	case WrMode: n = 0x00FF0300; break;
	case CrMode: n = 0x01FF0200; break;
	case DelMode: n = 0x02FF0100; break;
	case ExclMode: n = 0x03FF0000; break;
	}
	Pos = ModeLock + (n >> 16);
	Len = n & 0xFFFF;
}

bool ChangeLMode(LockMode Mode, WORD Kind, bool RdPref)
{
	FILE* h;
	longint pos, oldpos; WORD len, oldlen, count, d; longint w, w1; LockMode oldmode;
	bool result = false;
	if (!CFile->IsShared()) {         /*neu!!*/
		result = true; CFile->LMode = Mode; return result;
	}
	result = false; oldmode = CFile->LMode; h = CFile->Handle;
	if (oldmode >= WrMode) {
		if (Mode < WrMode) WrPrefixes();
		if (oldmode == ExclMode) { SaveCache(0); ClearCacheCFile(); }
		if (Mode < WrMode) ResetCFileUpdH();
	}
	w = 0; count = 0;
label1:
	if (Mode != NullMode)
		if (!TryLockH(h, TransLock, 1)) {
		label2:
			if (Kind == 2) return result; /*0 Kind-wait, 1-wait until ESC, 2-no wait*/
			count++;
			if (count <= spec.LockRetries) d = spec.LockDelay;
			else {
				d = spec.NetDelay; SetCPathVol();
				Set2MsgPar(CPath, LockModeTxt[Mode]);
				w1 = PushWrLLMsg(825, Kind = 1);
				if (w == 0) w = w1; else TWork.Delete(w1); LockBeep();
			}
			if (KbdTimer(spec.NetDelay, Kind)) goto label1;
			if (w != 0) PopW(w); return result;
		}
	if (oldmode != NullMode) {
		ModeLockBnds(oldmode, oldpos, oldlen); UnLockH(h, oldpos, oldlen);
	}
	if (Mode != NullMode) {
		ModeLockBnds(Mode, pos, len); if (!TryLockH(h, pos, len)) {
			if (oldmode != NullMode) TryLockH(h, oldpos, oldlen);
			UnLockH(h, TransLock, 1); goto label2;
		}
		UnLockH(h, TransLock, 1);
	}
	if (w != 0) PopW(w);
	CFile->LMode = Mode;
	if ((oldmode < RdMode) && (Mode >= RdMode) && RdPref) RdPrefixes();
	result = true;
	return result;
}
#else
bool ChangeLMode(LockMode Mode, WORD Kind, bool RdPref)
{
	CFile->LMode = Mode;
	return true;
}
#endif


void OldLMode(LockMode Mode)
{
	/* !!! with CFile^ do!!! */
#ifdef FandSQL
	if (CFile->IsSQLFile) { CFile->LMode = Mode; return; }
#endif
	if (CFile->Handle == nullptr) return;
	if (Mode != CFile->LMode) ChangeLMode(Mode, 0, true);
}

void RunErrorM(LockMode Md, WORD N)
{
	OldLMode(Md);
	RunError(N);
}

void CloseClearHCFile()
{
	/* !!! with CFile^ do!!! */
	CloseClearH(CFile->Handle);
	if (CFile->Typ == 'X') CloseClearH(CFile->XF->Handle);
	if (CFile->TF != nullptr) CloseClearH(CFile->TF->Handle);
}

void CloseGoExit()
{
	CloseClearHCFile();
	GoExit();
}

TFile::TFile(const TFile& orig)
{
	Format = orig.Format;
}

void TFile::Err(WORD n, bool ex)
{
	if (IsWork) {
		SetMsgPar(FandWorkTName);
		WrLLF10Msg(n);
		if (ex) GoExit();
	}
	else {
		CFileMsg(n, 'T');
		if (ex) CloseGoExit();
	}
}

void TFile::TestErr()
{
	if (HandleError != 0) Err(700 + HandleError, true);
}

longint TFile::UsedFileSize()
{
	if (Format == FptFormat) return FreePart * BlockSize;
	else return longint(MaxPage + 1) << MPageShft;
}

bool TFile::NotCached()
{
	return !IsWork && CFile->NotCached();
}

bool TFile::Cached()
{
	return !NotCached();
}

BYTE ByteMask[_MAX_INT_DIG];

const BYTE DblS = 8;
const BYTE FixS = 8;
BYTE Fix[FixS];
BYTE RealMask[DblS + 1];
BYTE Dbl[DblS];

void UnPack(void* PackArr, void* NumArr, WORD NoDigits)
{
}

void Pack(void* NumArr, void* PackArr, WORD NoDigits)
{
	BYTE* source = (BYTE*)NumArr;
	BYTE* target = (BYTE*)PackArr;
	WORD i;
	for (i = 1; i < (NoDigits >> 1); i++)
		target[i] = ((source[(i << 1) - 1] & 0x0F) << 4) || (source[i << 1] & 0x0F);
	if (NoDigits % 2 == 1)
		target[(NoDigits >> 1) + 1] = (source[NoDigits] & 0x0F) << 4;
}

double RealFromFix(void* FixNo, WORD FLen)
{
	unsigned char ff[9]{ 0 };
	unsigned char rr[9]{ 0 };
	memcpy(ff + 1, FixNo, FLen); // zacneme na indexu 1 podle Pascal. kodu

	bool neg = ((ff[1] & 0x80) != 0); // zaporne cislo urcuje 1 bit
	if (neg) {
		if (ff[1] == 0x80) {
			for (size_t i = 2; i <= FLen; i++) {
				if (ff[i] != 0x00) break;
				return 0.0;
			}
		}
		for (size_t i = 1; i <= FLen; i++) ff[i] = ~ff[i];
		ff[FLen]++;
		WORD I = FLen;
		while (ff[I] == 0) {
			I--;
			if (I > 0) ff[I]++;
		}
	}
	integer first = 1;
	while (ff[first] == 0) first++;
	if (first > FLen) return 0;

	integer lef = 0;
	unsigned char b = ff[first];
	while ((b & 0x80) == 0) {
		b = b << 1;
		lef++;
	}
	ff[first] = ff[first] & (0x7F >> lef);
	integer exp = ((FLen - first) << 3) - lef + 1030;
	if (lef == 7) first++;
	lef = (lef + 5) & 0x07;
	integer rig = 8 - lef;
	integer i = 8 - 1; // velikost double - 1
	if ((rig <= 4) && (first <= FLen)) {
		rr[i] = ff[first] >> rig;
		i--;
	}
	while ((i > 0) && (first < FLen)) {
		rr[i] = (ff[first] << lef) + (ff[first + 1] >> rig);
		i--;
		first++;
	}
	if ((first == FLen) && (i > 0)) {
		unsigned char t = ff[first] << lef;
		rr[i] = t;
	}
	rr[DblS - 1] = (rr[DblS - 1] & 0x0F) + ((exp & 0x0F) << 4);
	rr[DblS] = exp >> 4;
	if (neg) rr[DblS] = rr[DblS] || 0x80;
	return *(double*)&rr[1];
}

void FixFromReal(double r, void* FixNo, WORD FLen)
{
	unsigned char ff[9]{ 0 };
	unsigned char rr[9]{ 0 };
	// memcpy(ff + 1, FixNo, FLen); // zacneme na indexu 1 podle Pascal. kodu
	if (r > 0) r += 0.5;
	else r -= 0.5;
	memcpy(rr + 1, &r, 8);
	bool neg = ((rr[8] & 0x80) != 0); // zaporne cislo urcuje 1 bit
	integer exp = (rr[8 - 1] >> 4) + (WORD)((rr[8] & 0x7F) << 4);
	if (exp < 2047)
	{
		rr[8] = 0;
		rr[8 - 1] = rr[8 - 1] & 0x0F;
		if (exp > 0) rr[8 - 1] = rr[8 - 1] | 0x10;
		else exp++;
		exp -= 1023;
		if (exp > (FLen << 3) - 1) return; // OVERFLOW
		longint lef = (exp + 4) & 0x0007;
		longint rig = 8 - lef;
		if ((exp & 0x0007) > 3) exp += 4;
		longint first = 7 - (exp >> 3);
		int i = FLen;
		while ((first < 8) && (i > 0)) {
			ff[i] = (rr[first] >> rig) + (rr[first + 1] << lef);
			i--;
			first++;
		}
		if (i > 0) ff[i] = rr[first] >> rig;
		if (neg) {
			for (i = 1; i <= FLen; i++) ff[i] = ~ff[i];
			ff[FLen]++;
			i = FLen;
			while (ff[i] == 0) {
				i--;
				if (i > 0) ff[i]++;
			}
		}
	}
	memcpy(FixNo, &ff[1], FLen);
}

#ifdef FandNetV
const longint TransLock = 0x0A000501;  /* locked while state transition */
const longint ModeLock = 0x0A000000;  /* base for mode locking */
const longint RecLock = 0x0B000000;  /* base for record locking */
#endif

struct TT1Page
{
	WORD Signum = 0;
	WORD OldMaxPage = 0;
	longint FreePart = 0;
	bool Rsrvd1 = false, CompileProc = false, CompileAll = false;
	WORD IRec = 0;
	// potud se to nekoduje (13B)
	// odtud jsou polozky prohnany XORem
	longint FreeRoot = 0, MaxPage = 0;   /*eldest version=>array Pw[1..40] of char;*/
	double TimeStmp = 0.0;
	bool HasCoproc = false;
	char Rsrvd2[25]{ '\0' };
	char Version[4]{ '\0' };
	BYTE LicText[105]{ 0 };
	BYTE Sum = 0;
	char X1[295]{ '\0' };
	WORD LicNr = 0;
	char X2[11]{ '\0' };
	char PwNew[40]{ '\0' };
	BYTE Time = 0;

	void Load(BYTE* input512);
	void Save(BYTE* output512);
};

// nahraje nactenych 512B ze souboru do struktury
void TT1Page::Load(BYTE* input512)
{
	size_t index = 0;
	memcpy(&Signum, &input512[index], 2); index += 2;
	memcpy(&OldMaxPage, &input512[index], 2); index += 2;
	memcpy(&FreePart, &input512[index], 4); index += 4;
	memcpy(&Rsrvd1, &input512[index], 1); index++;
	memcpy(&CompileProc, &input512[index], 1); index++;
	memcpy(&CompileAll, &input512[index], 1); index++;
	memcpy(&IRec, &input512[index], 2); index += 2;
	memcpy(&FreeRoot, &input512[index], 4); index += 4;
	memcpy(&MaxPage, &input512[index], 4); index += 4;
	memcpy(&TimeStmp, &input512[index], 6); index += 6;
	memset(&TimeStmp + 6, 0, 2); // v pascalu to bylo 6B, tady je to 8B
	memcpy(&HasCoproc, &input512[index], 1); index++;
	memcpy(&Rsrvd2, &input512[index], 25); index += 25;
	memcpy(&Version, &input512[index], 4); index += 4;
	memcpy(&LicText, &input512[index], 105); index += 105;
	memcpy(&Sum, &input512[index], 1); index++;
	memcpy(&X1, &input512[index], 295); index += 295;
	memcpy(&LicNr, &input512[index], 2); index += 2;
	memcpy(&X2, &input512[index], 11); index += 11;
	memcpy(&PwNew, &input512[index], 40); index += 40;
	memcpy(&Time, &input512[index], 1); index++;
	// index by ted mel mit hodnotu 512
	if (index != 512) throw std::exception("Error in TT1Page::Load");
}

// nahraje nactenych 512B ze souboru do struktury
void TT1Page::Save(BYTE* output512)
{
	size_t index = 0;
	memcpy(&output512[index], &Signum, 2); index += 2;
	memcpy(&output512[index], &OldMaxPage, 2); index += 2;
	memcpy(&output512[index], &FreePart, 4); index += 4;
	memcpy(&output512[index], &Rsrvd1, 1); index++;
	memcpy(&output512[index], &CompileProc, 1); index++;
	memcpy(&output512[index], &CompileAll, 1); index++;
	memcpy(&output512[index], &IRec, 2); index += 2;
	memcpy(&output512[index], &FreeRoot, 4); index += 4;
	memcpy(&output512[index], &MaxPage, 4); index += 4;
	memcpy(&output512[index], &TimeStmp, 6); index += 6;
	//memset(&TimeStmp + 6, 0, 2); // v pascalu to bylo 6B, tady je to 8B
	memcpy(&output512[index], &HasCoproc, 1); index++;
	memcpy(&output512[index], &Rsrvd2, 25); index += 25;
	memcpy(&output512[index], &Version, 4); index += 4;
	memcpy(&output512[index], &LicText, 105); index += 105;
	memcpy(&output512[index], &Sum, 1); index++;
	memcpy(&output512[index], &X1, 295); index += 295;
	memcpy(&output512[index], &LicNr, 2); index += 2;
	memcpy(&output512[index], &X2, 11); index += 11;
	memcpy(&output512[index], &PwNew, 40); index += 40;
	memcpy(&output512[index], &Time, 1); index++;
	// index by ted mel mit hodnotu 512
	if (index != 512) throw std::exception("Error in TT1Page::Load");
}

void ResetCFileUpdH()
{
	/* !!! with CFile^ do!!! */
	ResetUpdHandle(CFile->Handle);
	if (CFile->Typ == 'X') ResetUpdHandle(CFile->XF->Handle);
	if (CFile->TF != nullptr) ResetUpdHandle(CFile->TF->Handle);
}

void ClearCacheCFile()
{
	// chache nepouzivame
	return;
	/* !!! with CFile^ do!!! */
	/*ClearCacheH(CFile->Handle);
	if (CFile->Typ == 'X') ClearCacheH(CFile->XF->Handle);
	if (CFile->TF != nullptr) ClearCacheH(CFile->TF->Handle);*/
}

bool TryLMode(LockMode Mode, LockMode& OldMode, WORD Kind)
{
	/* !!! with CFile^ do!!! */
	auto result = true;
#ifdef FandSQL
	if (CFile->IsSQLFile) {
		OldMode = CFile->LMode; if (Mode > CFile->LMode) CFile->LMode = Mode;
	}
	else
#endif
	{
		if (CFile->Handle == nullptr) OpenCreateF(Shared);
		OldMode = CFile->LMode;
		if (Mode > CFile->LMode) result = ChangeLMode(Mode, Kind, true);
	}
	return result;
}

LockMode NewLMode(LockMode Mode)
{
	LockMode md;
	TryLMode(Mode, md, 0);
	return md;
}

bool TryLockN(longint N, WORD Kind)
{
	longint w, w1; WORD m;
	pstring XTxt(3); XTxt = "CrX";
	auto result = true;
#ifdef FandSQL
	if (CFile->IsSQLFile) return result;
#endif
#ifdef FandNetV

	if (!CFile->IsShared) return result; w = 0;
label1:
	if (!TryLockH(CFile->Handle, RecLock + N, 1)) {
		if (Kind != 2) {   /*0 Kind-wait, 1-wait until ESC, 2-no wait*/
			m = 826;
			if (N == 0) { SetCPathVol(); Set2MsgPar(CPath, XTxt); m = 825; }
			w1 = PushWrLLMsg(m, Kind = 1);
			if (w == 0) w = w1;
			else TWork.Delete(w1);
			/*beep; don't disturb*/
			if (KbdTimer(spec.NetDelay, Kind)) goto label1;
		}
		result = false;
	}
	if (w != 0) PopW(w);
#endif
	return result;
}

void UnLockN(longint N)
{
	/* !!! with CFile^ do!!! */
#ifdef FandSQL
	if (CFile->IsSQLFile) return;
#endif
#ifdef FandNetV

	if ((CFile->Handle == nullptr) || !CFile->IsShared) return;
	UnLockH(CFile->Handle, RecLock + N, 1);
#endif
}


WORD RdPrefix()
{
	// NRs - celkovy pocet zaznamu v souboru; RLen - delka 1 zaznamu
	struct x6 { longint NRs = 0; WORD RLen = 0; } X6;
	struct x8 { WORD NRs = 0, RLen = 0; } X8;
	struct xD {
		BYTE Ver = 0; BYTE Date[3] = { 0,0,0 };
		longint NRecs = 0;
		WORD HdLen = 0; WORD RecLen = 0;
	} XD;
	auto result = 0xffff;
	/* !!! with CFile^ do!!! */
	bool cached = CFile->NotCached();
	switch (CFile->Typ) {
	case '8': {
		RdWrCache(true, CFile->Handle, cached, 0, 4, &X8);
		CFile->NRecs = X8.NRs;
		if (CFile->RecLen != X8.RLen) { return X8.RLen; }
		break;
	}
	case 'D': {
		RdWrCache(true, CFile->Handle, cached, 0, 12, &XD);
		CFile->NRecs = XD.NRecs;
		if ((CFile->RecLen != XD.RecLen)) { return XD.RecLen; }
		CFile->FrstDispl = XD.HdLen;
		break;
	}
	default: {
		RdWrCache(true, CFile->Handle, cached, 0, 6, &X6);
		CFile->NRecs = abs(X6.NRs);
		if ((X6.NRs < 0) && (CFile->Typ != 'X') || (X6.NRs > 0) && (CFile->Typ == 'X')
			|| (CFile->RecLen != X6.RLen)) {
			return X6.RLen;
		}
		break;
	}
	}
	return result;
}

void RdPrefixes()
{
	if (RdPrefix() != 0xffff) CFileError(883);
	/* !!! with CFile^ do!!! */ {
		if ((CFile->XF != nullptr) && (CFile->XF->Handle != nullptr)) CFile->XF->RdPrefix();
		if ((CFile->TF != nullptr)) CFile->TF->RdPrefix(false); }
}

void WrDBaseHd()
{
	DBaseHd* P = nullptr;

	FieldDescr* F;
	WORD n, y, m, d, w;
	pstring s;

	const char CtrlZ = '\x1a';

	P = (DBaseHd*)GetZStore(CFile->FrstDispl);
	char* PA = (char*)&P; // PA:CharArrPtr absolute P;
	F = CFile->FldD;
	n = 0;
	while (F != nullptr) {
		if ((F->Flg & f_Stored) != 0) {
			n++;
			{ // with P^.Flds[n]
				auto actual = P->Flds[n];
				switch (F->Typ) {
				case 'F': { actual.Typ = 'N'; actual.Dec = F->M; break; }
				case 'N': {actual.Typ = 'N'; break; }
				case 'A': {actual.Typ = 'C'; break; }
				case 'D': {actual.Typ = 'D'; break; }
				case 'B': {actual.Typ = 'L'; break; }
				case 'T': {actual.Typ = 'M'; break; }
				default:;
				}
				actual.Len = F->NBytes;
				actual.Displ = F->Displ;
				s = F->Name;
				for (size_t i = 1; i < s.length(); i++) s[i] = toupper(s[i]);
				StrLPCopy((char*)&actual.Name[1], s, 11);
			}
		}
		F = (FieldDescr*)F->Chain;
	}

	{ //with P^ do 
		if (CFile->TF != nullptr) {
			if (CFile->TF->Format == TFile::FptFormat) P->Ver = 0xf5;
			else P->Ver = 0x83;
		}
		else P->Ver = 0x03;

		P->RecLen = CFile->RecLen;
		SplitDate(Today(), d, m, y);
		P->Date[1] = BYTE(y - 1900);
		P->Date[2] = (BYTE)m;
		P->Date[3] = (BYTE)d;
		P->NRecs = CFile->NRecs;
		P->HdLen = CFile->FrstDispl;
		PA[(P->HdLen / 32) * 32 + 1] = m;
	}

	// with CFile^
	{
		bool cached = CFile->NotCached();
		RdWrCache(false, CFile->Handle, cached, 0, CFile->FrstDispl, (void*)&P);
		RdWrCache(false, CFile->Handle, cached,
			longint(CFile->NRecs) * CFile->RecLen + CFile->FrstDispl, 1, (void*)&CtrlZ);
	}

	ReleaseStore(P);
}

void WrPrefix()
{
	struct
	{
		longint NRs;
		WORD RLen;
	} Pfx6 = { 0, 0 };

	struct
	{
		WORD NRs;
		WORD RLen;
	} Pfx8 = { 0, 0 };

	if (IsUpdHandle(CFile->Handle))
	{
		bool cached = CFile->NotCached();
		switch (CFile->Typ)
		{
		case '8': {
			Pfx8.RLen = CFile->RecLen;
			Pfx8.NRs = CFile->NRecs;
			RdWrCache(false, CFile->Handle, cached, 0, 4, (void*)&Pfx8);
			break;
		}
		case 'D': {
			WrDBaseHd();
			break;
		}
		default: {
			Pfx6.RLen = CFile->RecLen;
			if (CFile->Typ == 'X') Pfx6.NRs = -CFile->NRecs;
			else Pfx6.NRs = CFile->NRecs;
			RdWrCache(false, CFile->Handle, cached, 0, 6, (void*)&Pfx6);
		}
		}
	}
}

void WrPrefixes()
{
	WrPrefix(); /*with CFile^ do begin*/
	if (CFile->TF != nullptr && IsUpdHandle(CFile->TF->Handle))
		CFile->TF->WrPrefix();
	if (CFile->Typ == 'X' && (CFile->XF)->Handle != nullptr
		&& /*{ call from CopyDuplF }*/ (IsUpdHandle(CFile->XF->Handle) || IsUpdHandle(CFile->Handle)))
		CFile->XF->WrPrefix();
}

// zmeni priponu na ._X_ a nastavi CPath
void CExtToX()
{
	CExt[2] = 'X';
	CPath = CDir + CName + CExt;
}

void TestCFileError()
{
	if (HandleError != 0) CFileError(700 + HandleError);
}

void TestCPathError()
{
	WORD n;
	if (HandleError != 0) {
		n = 700 + HandleError;
		if ((n == 705) && (CPath[CPath.length()] == '\\')) n = 840;
		SetMsgPar(CPath);
		RunError(n);
	}
}

// zmeni priponu souboru a nastavi CPath
void CExtToT()
{
	if (SEquUpcase(CExt, ".RDB")) CExt = ".TTT";
	else
		if (SEquUpcase(CExt, ".DBF"))
			if (CFile->TF->Format == TFile::FptFormat) CExt = ".FPT";
			else CExt = ".DBT";
		else CExt[2] = 'T';
	CPath = CDir + CName + CExt;
}

void XFNotValid()
{
	XFile* XF = CFile->XF;
	if (XF == nullptr) return;
	if (XF->Handle == nullptr) RunError(903);
	XF->SetNotValid();
}

void NegateESDI()
{
	// asm  jcxz @2; @1:not es:[di].byte; inc di; loop @1; @2:
}

void TestXFExist()
{
	XFile* xf = CFile->XF;
	if ((xf != nullptr) && xf->NotValid)
	{
		if (xf->NoCreate) CFileError(819);
		CreateIndexFile();
	}
}

longint XNRecs(KeyDPtr K)
{
	if ((CFile->Typ == 'X') && (K != nullptr))
	{
		TestXFExist();
		return CFile->XF->NRecs;
	}
	return CFile->NRecs;
}

//void ReadRec(longint N)
//{
//	/* with CFile^ do */
//	RdWrCache(true, CFile->Handle, CFile->NotCached(),
//		(N - 1) * CFile->RecLen + CFile->FrstDispl, CFile->RecLen, CRecPtr);
//}

void ReadRec(FileD* file, longint N, void* record)
{
	RdWrCache(true, file->Handle, file->NotCached(),
		(N - 1) * file->RecLen + file->FrstDispl, file->RecLen, record);
}

void WriteRec(longint N)
{
	RdWrCache(false, CFile->Handle, CFile->NotCached(),
		(N - 1) * CFile->RecLen + CFile->FrstDispl, CFile->RecLen, CRecPtr);
	CFile->WasWrRec = true;
}

void WriteRec(FileD* file, longint N, void* record)
{
	RdWrCache(false, file->Handle, file->NotCached(),
		(N - 1) * file->RecLen + file->FrstDispl, file->RecLen, record);
	file->WasWrRec = true;
}

void RecallRec(longint RecNr)
{
	TestXFExist();
	CFile->XF->NRecs++;
	KeyDPtr K = CFile->Keys;
	while (K != nullptr) { K->Insert(RecNr, false); K = K->Chain; }
	ClearDeletedFlag();
	WriteRec(RecNr);
}

void TryInsertAllIndexes(longint RecNr)
{
	void* p = nullptr;
	TestXFExist();
	MarkStore(p);
	KeyDPtr K = CFile->Keys;
	while (K != nullptr) {
		if (!K->Insert(RecNr, true)) goto label1; K = K->Chain;
	}
	CFile->XF->NRecs++;
	return;
label1:
	ReleaseStore(p);
	KeyDPtr K1 = CFile->Keys;
	while ((K1 != nullptr) && (K1 != K)) {
		K1->Delete(RecNr); K1 = K1->Chain;
	}
	SetDeletedFlag();
	WriteRec(RecNr);
	/* !!! with CFile->XF^ do!!! */
	if (CFile->XF->FirstDupl) {
		SetMsgPar(CFile->Name);
		WrLLF10Msg(828);
		CFile->XF->FirstDupl = false;
	}
}

void DeleteAllIndexes(longint RecNr)
{
	KeyDPtr K;
	K = CFile->Keys;
	while (K != nullptr) {
		K->Delete(RecNr);
		K = K->Chain;
	}
}

bool IsNullValue(void* p, WORD l)
{
	BYTE* pb = (BYTE*)p;
	for (size_t i = 0; i < l; i++)
	{
		if (pb[i] != 0xFF) return false;
	}
	return true;
}

// v CRecPtr se posune o F->Displ a vyète integer
longint _T(FieldDescr* F)
{
	return _T(F, (unsigned char*)CRecPtr, CFile->Typ);
}

longint _T(FieldDescr* F, unsigned char* data, char Typ)
{
	longint n = 0;
	integer err = 0;
	char* source = (char*)data + F->Displ;

	if (Typ == 'D')
	{
		// tváøíme se, že CRecPtr je pstring ...
		// TODO: toto je asi blbì, nutno opravit pøed 1. použitím
		pstring* s = (pstring*)CRecPtr;
		auto result = std::stoi(LeadChar(' ', *s));
		return result;
	}
	else
	{
		if (data == nullptr) return 0;
		return *(longint*)source;
	}
}

void T_(FieldDescr* F, longint Pos)
{
	pstring s;
	void* p = CRecPtr;
	char* source = (char*)p + F->Displ;
	longint* LP = (longint*)source;
	if ((F->Typ == 'T') && ((F->Flg & f_Stored) != 0)) {
		if (CFile->Typ == 'D')
			if (Pos == 0) FillChar(source, 10, ' ');
			else { str(Pos, s); Move(&s[1], source, 10); }
		else *LP = Pos;
	}
	else RunError(906);
}

void DelTFld(FieldDPtr F)
{
	longint n; LockMode md;
	n = _T(F);
	if (HasTWorkFlag()) TWork.Delete(n);
	else {
		md = NewLMode(WrMode); CFile->TF->Delete(n); OldLMode(md);
	}
	T_(F, 0);
}

void DelDifTFld(void* Rec, void* CompRec, FieldDPtr F)
{
	void* cr = CRecPtr;
	CRecPtr = CompRec;
	longint n = _T(F);
	CRecPtr = Rec;
	if (n != _T(F)) DelTFld(F);
	CRecPtr = cr;
}

void DelAllDifTFlds(void* Rec, void* CompRec)
{
	FieldDescr* F = CFile->FldD;
	while (F != nullptr)
	{
		if (F->Typ == 'T' && ((F->Flg & f_Stored) != 0)) DelDifTFld(Rec, CompRec, F);
		F = (FieldDescr*)F->Chain;
	}
}

void DeleteXRec(longint RecNr, bool DelT)
{
	TestXFExist();
	DeleteAllIndexes(RecNr);
	if (DelT) DelAllDifTFlds(CRecPtr, nullptr);
	SetDeletedFlag();
	WriteRec(RecNr);
	CFile->XF->NRecs--;
}

void OverWrXRec(longint RecNr, void* P2, void* P)
{
	XString x, x2; KeyDPtr K;
	CRecPtr = P2;
	if (DeletedFlag()) { CRecPtr = P; RecallRec(RecNr); return; }
	TestXFExist();
	K = CFile->Keys;
	while (K != nullptr) {
		CRecPtr = P; x.PackKF(K->KFlds); CRecPtr = P2; x2.PackKF(K->KFlds);
		if (x.S != x2.S) {
			K->Delete(RecNr); CRecPtr = P; K->Insert(RecNr, false);
		}
		K = K->Chain;
	}
	CRecPtr = P;
	WriteRec(RecNr);
}

void AddFFs(KeyDPtr K, pstring& s)
{
	WORD l = MinW(K->IndexLen + 1, 255);
	for (WORD i = s.length() + 1; i <= l; i++) s[i] = 0xff;
	s[0] = (char)l;
}

/// asi vytvori XStringy pro zacatek a konec (rozsah) vyhledavani
/// pokud se hleda interval v klici
void CompKIFrml(XKey* K, KeyInD* KI, bool AddFF)
{
	XString x;
	while (KI != nullptr) {
		bool b = x.PackFrml(KI->FL1, K->KFlds);
		KI->X1 = x.S;
		if (KI->FL2 != nullptr) x.PackFrml(KI->FL2, K->KFlds);
		if (AddFF) AddFFs(K, x.S);
		KI->X2 = x.S;
		KI = (KeyInD*)KI->Chain;
	}
}

const WORD Alloc = 2048;
const double FirstDate = 6.97248E+5;

void IncNRecs(longint N)
{
#ifdef FandDemo
	if (NRecs > 100) RunError(884);
#endif
	CFile->NRecs += N;
	SetUpdHandle(CFile->Handle);
	if (CFile->Typ == 'X') SetUpdHandle(CFile->XF->Handle);
}

void DecNRecs(longint N)
{
	/* !!! with CFile^ do!!! */
	CFile->NRecs -= N;
	SetUpdHandle(CFile->Handle);
	if (CFile->Typ == 'X') SetUpdHandle(CFile->XF->Handle);
	CFile->WasWrRec = true;
}

void SeekRec(longint N)
{
	CFile->IRec = N;
	if (CFile->XF == nullptr) CFile->Eof = N >= CFile->NRecs;
	else CFile->Eof = N >= CFile->XF->NRecs;
}

void PutRec()
{
	/* !!! with CFile^ do!!! */
	CFile->NRecs++;
	RdWrCache(false, CFile->Handle, CFile->NotCached(),
		longint(CFile->IRec) * CFile->RecLen + CFile->FrstDispl, CFile->RecLen, CRecPtr);
	CFile->IRec++;
	CFile->Eof = true;
}

void CreateRec(longint N)
{
	IncNRecs(1);
	void* cr = CRecPtr;
	CRecPtr = GetRecSpace();
	for (longint i = CFile->NRecs - 1; i > N; i--) {
		ReadRec(CFile, i, CRecPtr);
		WriteRec(i + 1);
	}
	ReleaseStore(CRecPtr);
	CRecPtr = cr;
	WriteRec(N);
}

void DeleteRec(longint N)
{
	DelAllDifTFlds(CRecPtr, nullptr);
	for (longint i = N; i < CFile->NRecs - 1; i++) {
		ReadRec(CFile, i + 1, CRecPtr);
		WriteRec(i);
	}
	DecNRecs(1);
}

void LongS_(FieldDescr* F, LongStr* S)
{
	longint Pos; LockMode md;

	if ((F->Flg & f_Stored) != 0) {
		if (S->LL == 0) T_(F, 0);
		else {
			if ((F->Flg & f_Encryp) != 0) Code(S->A, S->LL);
#ifdef FandSQL
			if (CFile->IsSQLFile) { SetTWorkFlag; goto label1; }
			else
#endif
				if (HasTWorkFlag())
					label1:
			Pos = TWork.Store(S);
				else {
					md = NewLMode(WrMode);
					Pos = CFile->TF->Store(S);
					OldLMode(md);
				}
			if ((F->Flg & f_Encryp) != 0) Code(S->A, S->LL);
			T_(F, Pos);
		}
	}
}

void S_(FieldDescr* F, std::string S, void* record)
{
	const BYTE LeftJust = 1;
	BYTE* pRec = nullptr;

	if ((F->Flg & f_Stored) != 0)
	{
		if (record == nullptr) { pRec = (BYTE*)CRecPtr + F->Displ; }
		else { pRec = (BYTE*)record + F->Displ; }
		integer L = F->L;
		integer M = F->M;
		switch (F->Typ) {
		case 'A': {
			if (M == LeftJust) {
				// doplnime mezery zprava
				memcpy(pRec, S.c_str(), S.length());
				memset(&pRec[S.length()], ' ', F->L - S.length());
			}
			else {
				// doplnime mezery zleva
				memset(pRec, ' ', F->L - S.length());
				memcpy(&pRec[F->L - S.length()], S.c_str(), S.length());
			}
			break;
		}
		case 'N': {
			BYTE tmpArr[80]{ 0 };
			if (M == LeftJust) {
				// doplnime nuly zprava
				memcpy(tmpArr, S.c_str(), S.length());
				memset(&tmpArr[F->L - S.length()], '0', F->L - S.length());
			}
			else {
				// doplnime mezery zleva
				memset(tmpArr, ' ', F->L - S.length());
				memcpy(&tmpArr[F->L - S.length()], S.c_str(), S.length());
			}
			bool odd = F->L % 2 == 1; // lichy pocet znaku
			for (size_t i = 0; i < F->NBytes; i++) {
				if (odd && i == F->NBytes - 1) {
					pRec[i] = ((tmpArr[2 * i] - 0x30) << 4);
				}
				else {
					pRec[i] = ((tmpArr[2 * i] - 0x30) << 4) + (tmpArr[2 * i + 1] - 0x30);
				}
			}

			//while (S.length() < L)
			//	if (M == LeftJust) S = S + "0";
			//	else
			//	{
			//		pstring oldS = S;
			//		S = " ";
			//		S += oldS;
			//	}
			//integer i = 1;
			//if ((S.length() > L) && (M != LeftJust)) i = S.length() + 1 - L;
			////Pack(&S[i], p, L);
			break;
		}
		case 'T': {
			LongStr* ss = CopyToLongStr(S);
			LongS_(F, ss);
			ReleaseStore(ss);
			break;
		}
		}
	}
}

void ZeroAllFlds()
{
	FillChar(CRecPtr, CFile->RecLen, 0);
	FieldDPtr F = CFile->FldD;
	while (F != nullptr) {
		if (((F->Flg & f_Stored) != 0) && (F->Typ == 'A')) S_(F, "");
		F = (FieldDescr*)F->Chain;
	}
}

bool LinkLastRec(FileD* FD, longint& N, bool WithT)
{
	CFile = FD;
	CRecPtr = GetRecSpace();
	LockMode md = NewLMode(RdMode);
	auto result = true;
#ifdef FandSQL
	if (FD->IsSQLFile)
	{
		if (Strm1->SelectXRec(nullptr, nullptr, _equ, WithT)) N = 1; else goto label1;
	}
	else
#endif
	{
		N = CFile->NRecs;
		if (N == 0) {
		label1:
			ZeroAllFlds();
			result = false;
			N = 1;
		}
		else ReadRec(CFile, N, CRecPtr);
	}
	OldLMode(md);
	return result;
}

// ulozi hodnotu parametru do souboru
void AsgnParFldFrml(FileD* FD, FieldDescr* F, FrmlElem* Z, bool Ad)
{
//#ifdef _DEBUG
	std::string FileName = FD->FullName;
	std::string Varible = F->Name;
//#endif
	void* p = nullptr; longint N = 0; LockMode md; bool b = false;
	FileD* cf = CFile; void* cr = CRecPtr; CFile = FD;
#ifdef FandSQL
	if (CFile->IsSQLFile) {
		CRecPtr = GetRecSpace; ZeroAllFlds; AssgnFrml(F, Z, true, Ad);
		Strm1->UpdateXFld(nullptr, nullptr, F); ClearRecSpace(CRecPtr)
	}
	else
#endif
	{
		md = NewLMode(WrMode);
		if (!LinkLastRec(CFile, N, true)) {
			IncNRecs(1);
			WriteRec(CFile, N, CRecPtr);
		}
		AssgnFrml(F, Z, true, Ad);
		WriteRec(CFile, N, CRecPtr);
		OldLMode(md);
	}
	ReleaseStore(CRecPtr);
	CFile = cf; CRecPtr = cr;
}

bool SearchKey(XString& XX, KeyDPtr Key, longint& NN)
{
	longint R = 0;
	XString x;

	auto bResult = false;
	longint L = 1;
	integer Result = _gt;
	NN = CFile->NRecs;
	longint N = NN;
	if (N == 0) return bResult;
	KeyFldDPtr KF = Key->KFlds;
	do {
		if (Result == _gt) R = N;
		else L = N + 1;
		N = (L + R) / 2;
		ReadRec(CFile, N, CRecPtr);
		x.PackKF(KF);
		Result = CompStr(x.S, XX.S);
	} while (!((L >= R) || (Result == _equ)));
	if ((N == NN) && (Result == _lt)) NN++;
	else {
		if (Key->Duplic && (Result == _equ))
			while (N > 1) {
				N--;
				ReadRec(CFile, N, CRecPtr);
				x.PackKF(KF);
				if (CompStr(x.S, XX.S) != _equ) {
					N++;
					ReadRec(CFile, N, CRecPtr);
					goto label1;
				}
			}
	label1:  NN = N;
	}
	if ((Result == _equ) || Key->Intervaltest && (Result == _gt))
		bResult = true;
	return bResult;
}

LongStr* _LongS(FieldDescr* F)
{
	void* P = CRecPtr;
	char* source = (char*)P + F->Displ;
	LongStr* S = nullptr; longint Pos = 0; integer err = 0;
	LockMode md; WORD l = 0;
	if ((F->Flg & f_Stored) != 0) {
		l = F->L;
		switch (F->Typ)
		{
		case 'A':		// znakovy retezec max. 255 znaku
		case 'N': {		// ciselny retezec max. 79 znaku
			S = new LongStr(l);
			S->LL = l;
			if (F->Typ == 'A') {
				Move(source, &S->A[0], l);
				if ((F->Flg & f_Encryp) != 0) Code(S->A, l);
				if (IsNullValue(S, l)) {
					S->LL = 0;
					//ReleaseAfterLongStr(S);
				}
			}
			else if (IsNullValue(source, F->NBytes)) {
				S->LL = 0;
				//ReleaseAfterLongStr(S);
			}
			else
			{
				// jedna je o typ N - prevedeme cislo na znaky
				// UnPack(P, S->A, l);
				for (size_t i = 0; i < F->NBytes; i++) {
					S->A[2 * i] = ((BYTE)source[i] >> 4) + 0x30;
					S->A[2 * i + 1] = ((BYTE)source[i] & 0x0F) + 0x30;
				}
			}
			break;
		}
		case 'T': {		// volny text max. 65k
			if (HasTWorkFlag()) S = TWork.Read(1, _T(F));
			else {
				md = NewLMode(RdMode);
				S = CFile->TF->Read(1, _T(F));
				OldLMode(md);
			}
			if ((F->Flg & f_Encryp) != 0) Code(S->A, S->LL);
			if (IsNullValue(S->A, S->LL))
			{
				S->LL = 0;
				// ReleaseAfterLongStr(S);
			}
			break;
		}
		}
		return S;
	}
	return RunLongStr(F->Frml);
}

// z CRecPtr vyète øetìzec o délce F->L z pozice F->Displ
pstring _ShortS(FieldDescr* F)
{
	void* P = CRecPtr;
	char* source = (char*)P + F->Displ;
	pstring S;
	if ((F->Flg & f_Stored) != 0) {
		WORD l = F->L;
		S[0] = l;
		switch (F->Typ) {
		case 'A':		// znakovy retezec max. 255 znaku
		case 'N': {		// ciselny retezec max. 79 znaku
			if (F->Typ == 'A') {
				Move(source, &S[1], l);
				if ((F->Flg & f_Encryp) != 0) Code(&S[1], l);
				if (IsNullValue(&S[2], l)) FillChar(&S[0], l, ' ');
			}
			else if (IsNullValue(source, F->NBytes)) FillChar(&S[0], l, ' ');
			else
			{
				// nebudeme volat, zøejmìní není potøeba
				// UnPack(P, (WORD*)S[0], l);
				for (size_t i = 0; i < l; i++) {
					// kolikaty byte?
					size_t iB = i / 2;
					// leva nebo prava cislice?
					if (i % 2 == 0) {
						S[i + 1] = ((unsigned char)source[iB] >> 4) + 0x30;
					}
					else {
						S[i + 1] = (source[iB] & 0x0F) + 0x30;
					}
				}
			}
			break;
		}
		case 'T': {		// volny text max. 65k
			LongStrPtr ss = _LongS(F);
			if (ss->LL > 255) S = S.substr(0, 255);
			else S = S.substr(0, ss->LL);
			Move(&ss[0], &S[0], S.length());
			ReleaseStore(ss);
			break; };
		default:;
		}
		return S;
	}
	return RunShortStr(F->Frml);
}

double _RforD(FieldDPtr F, void* P)
{
	pstring s; integer err;
	double r = 0; s[0] = F->NBytes;
	Move(P, &s[1], s.length());
	switch (F->Typ) {
	case 'F': { ReplaceChar(s, ',', '.');
		if ((F->Flg & f_Comma) != 0) {
			integer i = s.first('.');
			if (i > 0) s.Delete(i, 1);
		}
		val(LeadChar(' ', TrailChar(' ', s)), r, err);
		break;
	}
	case 'D': r = ValDate(s, "YYYYMMDD"); break;
	}
	return r;
}

double _R(FieldDescr* F)
{
	void* P = CRecPtr;
	char* source = (char*)P + F->Displ;
	double result = 0.0;
	double r;

	if ((F->Flg & f_Stored) != 0) {
		if (CFile->Typ == 'D') result = _RforD(F, source);
		else switch (F->Typ) {
		case 'F': { // FIX CISLO (M,N)
			r = RealFromFix(source, F->NBytes);
			if ((F->Flg & f_Comma) == 0) result = r / Power10[F->M];
			else result = r;
			break;
		}
		case 'D': { // DATUM DD.MM.YY
			if (CFile->Typ == '8') {
				if (*(integer*)source == 0) result = 0.0;
				else result = *(integer*)source + FirstDate;
			}
			else goto label1;
			break;
		}
		case 'R': {
		label1:
			if (P == nullptr) result = 0;
			else {
				result = Real48ToDouble(source);
			}
		}
		}
	}
	else return RunReal(F->Frml);
	return result;
}

// vrací BOOL ze záznamu CRecPtr na pozici F->Displ
bool _B(FieldDescr* F)
{
	bool result = false;
	void* p = CRecPtr;
	unsigned char* CP = (unsigned char*)p + F->Displ;
	if ((F->Flg & f_Stored) != 0) {
		if (CFile->Typ == 'D') result = *CP == 'Y' || *CP == 'y' || *CP == 'T' || *CP == 't';
		else if ((*CP == '\0') || (*CP == 0xFF)) result = false;
		else result = true;
	}
	else result = RunBool(F->Frml);
	return result;
}

void R_(FieldDescr* F, double R)
{
	void* p = CRecPtr;
	BYTE* bp = (BYTE*)p + F->Displ;
	pstring s; WORD m = 0; longint l = 0;
	if ((F->Flg & f_Stored) != 0) {
		m = F->M;
		switch (F->Typ) {
		case 'F': {
			if (CFile->Typ == 'D') {
				if ((F->Flg & f_Comma) != 0) R = R / Power10[m];
				str(F->NBytes, s);
				Move(&s[1], bp, F->NBytes);
			}
			else {
				if ((F->Flg & f_Comma) == 0) R = R * Power10[m];
				FixFromReal(R, bp, F->NBytes);
			}
			break;
		}
		case 'D': {
			switch (CFile->Typ) {
			case '8': {
				if (trunc(R) == 0) *(long*)&bp = 0;
				else *(long*)bp = trunc(R - FirstDate);
				break;
			}
			case 'D': {
				s = StrDate(R, "YYYYMMDD");
				Move(&s[1], bp, 8);
				break;
			}
			default: {
				auto r48 = DoubleToReal48(R);
				for (size_t i = 0; i < 6; i++) {
					bp[i] = r48[i];
				}
				break;
			}
			}
			break;
		}
		case 'R': {
			auto r48 = DoubleToReal48(R);
			for (size_t i = 0; i < 6; i++) {
				bp[i] = r48[i];
			}
			break;
		}
		}
	}
}

void B_(FieldDescr* F, bool B)
{
	void* p = CRecPtr;
	char* pB = (char*)p + F->Displ;
	if ((F->Typ == 'B') && ((F->Flg & f_Stored) != 0)) {
		if (CFile->Typ == 'D')
		{
			if (B) *pB = 'T';
			else *pB = 'F';
		}
		else *pB = B;
	}
}

// zrejme zajistuje pristup do jine tabulky (cizi klic)
bool LinkUpw(LinkDPtr LD, longint& N, bool WithT)
{
	KeyFldD* KF;
	FieldDescr* F, * F2;
	bool LU = false; LockMode md;
	//pstring s; 
	double r = 0.0;
	bool b = false;
	//XString* x = (XString*)&s;
	XString x;

	FileD* ToFD = LD->ToFD;
	FileD* CF = CFile;
	void* CP = CRecPtr;
	KeyD* K = LD->ToKey;
	KeyFldD* Arg = LD->Args;
	x.PackKF(Arg);
	CFile = ToFD;
	void* RecPtr = GetRecSpace();
	CRecPtr = RecPtr;
#ifdef FandSQL
	if (CFile->IsSQLFile) {
		LU = Strm1->SelectXRec(K, @X, _equ, WithT); N = 1;
		if (LU) goto label2; else goto label1;
	}
#endif
	md = NewLMode(RdMode);
	if (ToFD->Typ == 'X') {
		TestXFExist();
		LU = K->SearchIntvl(x, false, N);
	}
	else if (CFile->NRecs == 0) { LU = false; N = 1; }
	else LU = SearchKey(x, K, N);
	if (LU) ReadRec(CFile, N, CRecPtr);
	else {
	label1:
		ZeroAllFlds();
		KF = K->KFlds;
		while (Arg != nullptr) {
			F = Arg->FldD;
			F2 = KF->FldD;
			CFile = CF; CRecPtr = CP;
			if ((F2->Flg & f_Stored) != 0)
				switch (F->FrmlTyp) {
				case 'S': {
					x.S = _ShortS(F);
					CFile = ToFD; CRecPtr = RecPtr;
					S_(F2, x.S);
					break;
				}
				case 'R': {
					r = _R(F);
					CFile = ToFD; CRecPtr = RecPtr;
					R_(F2, r);
					break;
				}
				case 'B': {
					b = _B(F);
					CFile = ToFD; CRecPtr = RecPtr;
					B_(F2, b);
					break;
				}
				}
			Arg = (KeyFldD*)Arg->Chain; KF = (KeyFldD*)KF->Chain;
		}
		CFile = ToFD;
		CRecPtr = RecPtr;
	}
label2:
	auto result = LU;
#ifdef FandSQL
	if (!CFile->IsSQLFile)
#endif
		OldLMode(md);
	return result;
}

void AssignNRecs(bool Add, longint N)
{
	longint OldNRecs; LockMode md;
#ifdef FandSQL
	if (CFile->IsSQLFile) {
		if ((N = 0) && !Add) Strm1->DeleteXRec(nullptr, nullptr, false); return;
	}
#endif
	md = NewLMode(DelMode); OldNRecs = CFile->NRecs;
	if (Add) N = N + OldNRecs;
	if ((N < 0) || (N == OldNRecs)) goto label1;
	if ((N == 0) && (CFile->TF != nullptr)) CFile->TF->SetEmpty();
	if (CFile->Typ == 'X')
		if (N == 0) {
			CFile->NRecs = 0;
			SetUpdHandle(CFile->Handle);
			XFNotValid();
			goto label1;
		}
		else {
			SetMsgPar(CFile->Name);
			RunErrorM(md, 821);
		}
	if (N < OldNRecs) {
		DecNRecs(OldNRecs - N);
		goto label1;
	}
	CRecPtr = GetRecSpace();
	ZeroAllFlds();
	SetDeletedFlag();
	IncNRecs(N - OldNRecs);
	for (longint i = OldNRecs + 1; i < N; i++) WriteRec(i);
	ReleaseStore(CRecPtr);
label1:
	OldLMode(md);
}

void ClearRecSpace(void* p)
{
	FieldDescr* f = nullptr;
	void* cr = nullptr;
	if (CFile->TF != nullptr) {
		cr = CRecPtr;
		CRecPtr = p;
		if (HasTWorkFlag()) {
			f = CFile->FldD;
			while (f != nullptr) {
				if (((f->Flg & f_Stored) != 0) && (f->Typ == 'T')) {
					TWork.Delete(_T(f));
					T_(f, 0);
				}
				f = (FieldDescr*)f->Chain;
			}
		}
		CRecPtr = cr;
	}
}

void DelTFlds()
{
	FieldDescr* F = CFile->FldD;
	while (F != nullptr) {
		if (((F->Flg & f_Stored) != 0) && (F->Typ == 'T')) DelTFld(F);
		F = (FieldDescr*)F->Chain;
	}
}

void CopyRecWithT(void* p1, void* p2)
{
	Move(p1, p2, CFile->RecLen);
	FieldDescr* F = CFile->FldD;
	while (F != nullptr) {
		if ((F->Typ == 'T') && ((F->Flg & f_Stored) != 0)) {
			TFilePtr tf1 = CFile->TF; TFilePtr tf2 = tf1; CRecPtr = p1;
			if ((tf1->Format != TFile::T00Format)) {
				LongStrPtr s = _LongS(F);
				CRecPtr = p2; LongS_(F, s);
				ReleaseStore(s);
			}
			else {
				if (HasTWorkFlag()) tf1 = &TWork;
				longint pos = _T(F);
				CRecPtr = p2;
				if (HasTWorkFlag()) tf2 = &TWork;
				pos = CopyTFString(tf2, CFile, tf1, pos);
				T_(F, pos);
			}
		}
		F = (FieldDescr*)F->Chain;
	}
}

void Code(void* A, WORD L)
{
	BYTE* pb = (BYTE*)A;
	for (int i = 0; i < L; i++)
	{
		pb[i] = pb[i] ^ 0xAA;
	}
}

void RandIntByBytes(longint& nr)
{
	BYTE* byte = (BYTE*)&nr;
	for (size_t i = 0; i < 4; i++)
	{
		byte[i] = byte[i] ^ Random(255); //randomice6s4173[randIndex++];
	}
}

void RandWordByBytes(WORD& nr)
{
	BYTE* byte = (BYTE*)&nr;
	for (size_t i = 0; i < 2; i++)
	{
		byte[i] = byte[i] ^ Random(255); //randomice6s4173[randIndex++];
	}
}

void RandReal48ByBytes(double& nr)
{
	BYTE* byte = (BYTE*)&nr;
	for (size_t i = 0; i < 6; i++)
	{
		byte[i] = byte[i] ^ Random(255); //randomice6s4173[randIndex++];
	}
}

void RandBooleanByBytes(bool& nr)
{
	BYTE* byte = (BYTE*)&nr;
	for (size_t i = 0; i < sizeof(nr); i++)
	{
		byte[i] = byte[i] ^ Random(255); //randomice6s4173[randIndex++];
	}
}

void RandArrayByBytes(void* arr, size_t len)
{
	BYTE* byte = (BYTE*)arr;
	for (size_t i = 0; i < len; i++)
	{
		byte[i] = byte[i] ^ Random(255); //randomice6s4173[randIndex++];
	}
}


void TFile::RdPrefix(bool Chk)
{
	TT1Page T;
	longint* TNxtAvailPage = (longint*)&T; /* .DBT */
	struct stFptHd { longint FreePart = 0; WORD X = 0, BlockSize = 0; }; /* .FPT */
	stFptHd* FptHd = (stFptHd*)&T;
	BYTE sum = 0; longint FS = 0, ML = 0, RS = 0; WORD i = 0, n = 0;
	if (Chk) {
		FS = FileSizeH(Handle);
		if (FS <= 512) {
			FillChar(PwCode, 40, '@');
			Code(PwCode, 40);
			SetEmpty();
			return;
		}
	}
	BYTE header512[512];
	RdWrCache(true, Handle, NotCached(), 0, 512, header512);
	srand(RS);
	LicenseNr = 0;
	if (Format == DbtFormat) {
		MaxPage = *TNxtAvailPage - 1;
		GetMLen();
		return;
	}
	if (Format == FptFormat) {
		FreePart = SwapLong((*FptHd).FreePart);
		BlockSize = Swap((*FptHd).BlockSize);
		return;
	}

	// nactena data jsou porad v poli, je nutne je nahrat do T
	T.Load(header512);

	// Move(&T.FreePart, &FreePart, 23);
	FreePart = T.FreePart; // 4B
	Reserved = T.Rsrvd1; // 1B
	CompileProc = T.CompileProc; // 1B
	CompileAll = T.CompileAll; // 1B
	IRec = T.IRec; // 2B
	FreeRoot = T.FreeRoot; // 4B
	MaxPage = T.MaxPage; // 4B
	TimeStmp = T.TimeStmp; // 6B v Pascalu, 8B v C++ 

	if (!IsWork && (CFile == Chpt) && ((T.HasCoproc != HasCoproc) ||
		(CompArea(Version, T.Version, 4) != _equ))) CompileAll = true;
	if (T.OldMaxPage == 0xffff) goto label1;
	else {
		FreeRoot = 0;
		if (FreePart > 0) {
			if (!Chk) FS = FileSizeH(Handle); ML = FS;
			MaxPage = (FS - 1) >> MPageShft; GetMLen();
		}
		else {
			FreePart = -FreePart; MaxPage = T.OldMaxPage;
		label1:
			GetMLen();
			ML = MLen;
			if (!Chk) FS = ML;
		}
	}
	if (IRec >= 0x6000) {
		IRec = IRec - 0x2000;
		if (!IsWork && (CFile->Typ == '0')) LicenseNr = T.LicNr;
	}
	if (IRec >= 0x4000) {
		IRec = IRec - 0x4000;
		RandSeed = ML + T.Time;
		RandIntByBytes(T.FreeRoot);
		RandIntByBytes(T.MaxPage);
		RandReal48ByBytes(T.TimeStmp);
		RandBooleanByBytes(T.HasCoproc);
		RandArrayByBytes(T.Rsrvd2, 25);
		RandArrayByBytes(T.Version, 4);
		RandArrayByBytes(T.LicText, 105);
		RandArrayByBytes(&T.Sum, 1);
		RandArrayByBytes(T.X1, 295);
		RandWordByBytes(T.LicNr);
		RandArrayByBytes(T.X2, 11);
		RandArrayByBytes(T.PwNew, 40);
		Move(T.PwNew, PwCode, 20);
		Move(&T.PwNew[20], Pw2Code, 20);
	}
	else {
		RandSeed = ML + T.Time;
		RandIntByBytes(T.FreeRoot);
		RandIntByBytes(T.MaxPage);
		RandReal48ByBytes(T.TimeStmp);
		RandBooleanByBytes(T.HasCoproc);
		RandArrayByBytes(T.Rsrvd2, 25);
		Move(T.PwNew, PwCode, 20);
		Move(&T.PwNew[20], Pw2Code, 20);
	}
	Code(PwCode, 20);
	Code(Pw2Code, 20);
	if ((FreePart < MPageSize) || (FreePart > ML) || (FS < ML) ||
		(FreeRoot > MaxPage) || (MaxPage == 0)) {
		Err(893, false);
		MaxPage = (FS - 1) >> MPageShft;
		FreeRoot = 0;
		GetMLen();
		FreePart = NewPage(true);
		SetUpdHandle(Handle);
	}
	//FillChar(&T, 512, 0);
	srand(RS);
}

void TFile::WrPrefix()
{
	TT1Page T;
	longint* TNxtAvailPage = (longint*)&T;		/* .DBT */
	struct stFptHd { longint FreePart = 0; WORD X = 0, BlockSize = 0; }; /* .FPT */
	stFptHd* FptHd = (stFptHd*)&T;
	char Pw[40];
	// BYTE absolute 0 Time:0x46C; TODO: TIMER
	WORD i = 0, n = 0;
	BYTE sum = 0; longint RS = 0;
	const PwCodeArr EmptyPw = { '@','@','@','@','@','@','@','@','@','@','@','@','@','@','@','@','@','@','@','@' };

	if (Format == DbtFormat) {
		memset(&T, ' ', sizeof(T));
		*TNxtAvailPage = MaxPage + 1;
		goto label1;
	}
	if (Format == FptFormat) {
		memset(&T, 0, sizeof(T));
		(*FptHd).FreePart = SwapLong(FreePart);
		(*FptHd).BlockSize = Swap(BlockSize);
		goto label1;
	}
	memset(&T, '@', sizeof(T));
	Move(PwCode, Pw, 40);
	Code(Pw, 40);
	RandSeed = RS;
	if (LicenseNr != 0)
		for (i = 0; i < 20; i++)
			Pw[i] = static_cast<char>(Random(255));
	n = 0x4000;
	// TODO: T.Time = Time;
	Move(Pw, T.PwNew, 40);
	RandSeed = MLen + T.Time; // srand(MLen + T.Time);
	// for (i = 14; i < 511; i++) TX[i] = TX[i] ^ Random(255);
	RandIntByBytes(T.FreeRoot);
	RandIntByBytes(T.MaxPage);
	RandReal48ByBytes(T.TimeStmp);
	RandBooleanByBytes(T.HasCoproc);
	RandArrayByBytes(T.Rsrvd2, 25);
	RandArrayByBytes(T.Version, 4);
	RandArrayByBytes(T.LicText, 105);
	RandArrayByBytes(&T.Sum, 1);
	RandArrayByBytes(T.X1, 295);
	RandWordByBytes(T.LicNr);
	RandArrayByBytes(T.X2, 11);
	RandArrayByBytes(T.PwNew, 40);

	T.LicNr = LicenseNr;
	if (LicenseNr != 0) {
		n = 0x6000;
		sum = T.LicNr;
		for (i = 0; i < 105; i++) sum += T.LicText[i];
		T.Sum = sum;
	}
	//Move(&FreePart, &T.FreePart, 23);
	T.FreePart = FreePart; // 8B
	T.Rsrvd1 = Reserved; // 1B
	T.CompileProc = CompileProc; // 1B
	T.CompileAll = CompileAll; // 1B
	T.IRec = IRec; // 2B
	T.FreeRoot = FreeRoot; // 4B
	T.MaxPage = MaxPage; // 4B
	T.TimeStmp = TimeStmp; // 6B Pascal

	T.OldMaxPage = 0xffff;
	T.Signum = 1;
	T.IRec += n;
	memcpy(T.Version, Version, 4); // Move(&Version[1], &T.Version, 4);
	T.HasCoproc = HasCoproc;
	RandSeed = RS;
label1:
	BYTE header512[512]{ 0 };
	T.Save(header512);
	RdWrCache(false, Handle, NotCached(), 0, 512, header512);
}

void TFile::SetEmpty()
{
	BYTE X[MPageSize];
	integer* XL = (integer*)&X;
	if (Format == DbtFormat) { MaxPage = 0; WrPrefix(); return; }
	if (Format == FptFormat) { FreePart = 8; BlockSize = 64; WrPrefix(); return; }
	FreeRoot = 0; MaxPage = 1; FreePart = MPageSize; MLen = 2 * MPageSize;
	WrPrefix();
	//FillChar(X, MPageSize, 0); 
	*XL = -510;
	RdWrCache(false, Handle, NotCached(), MPageSize, MPageSize, X);
}

void TFile::Create()
{
	Handle = OpenH(_isoverwritefile, Exclusive);
	TestErr();
	IRec = 1; LicenseNr = 0;
	FillChar(PwCode, 40, '@');
	Code(PwCode, 40);
	SetEmpty();
}

longint TFile::NewPage(bool NegL)
{
	longint PosPg;
	BYTE X[MPageSize];
	longint* L = (longint*)&X;
	if (FreeRoot != 0) {
		PosPg = FreeRoot << MPageShft;
		RdWrCache(true, Handle, NotCached(), PosPg, 4, &FreeRoot);
		if (FreeRoot > MaxPage) {
			Err(888, false);
			FreeRoot = 0; goto label1;
		}
	}
	else {
	label1:
		MaxPage++; MLen += MPageSize; PosPg = MaxPage << MPageShft;
	}
	//FillChar(X, MPageSize, 0); 
	if (NegL) *L = -510;
	RdWrCache(false, Handle, NotCached(), PosPg, MPageSize, X);
	return PosPg;
}

void TFile::ReleasePage(longint PosPg)
{
	BYTE X[MPageSize - 1];
	longint* Next = (longint*)&X;
	//FillChar(X, MPageSize, 0); 
	*Next = FreeRoot;
	RdWrCache(false, Handle, NotCached(), PosPg, MPageSize, X);
	FreeRoot = PosPg >> MPageShft;
}

void TFile::Delete(longint Pos)
{
	// funkce smaze cast T00 nebo TTT souboru
	// s ohledem na to, ze v TTT souboru jsou predkompilovana data
	// a tato data pak nejsou nicim nahrazena (novou kompilaci neumime ulozit)
	// dojde ke ztrate dat

	// proto budeme kontrolovat, zda se jedna o TTT soubor a pokud ano,
	// tak koncime a nic mazat nebudeme

	std::string name = CFile->Name;
	if (name.find("ucto2020") == std::string::npos) return;
	if (name.find("MODUL") == std::string::npos) return;

	longint PosPg = 0, NxtPg = 0; WORD PosI = 0; integer N = 0; WORD l = 0;
	BYTE X[MPageSize]{ 0 };
	integer* XL = (integer*)&X;
	WORD* wp = nullptr;

	bool IsLongTxt = false;
	if (Pos <= 0) return;
	if ((Format != T00Format) || NotCached()) return;
	if ((Pos < MPageSize) || (Pos >= MLen)) { Err(889, false); return; }
	PosPg = Pos & (0xFFFFFFFF << MPageShft);
	PosI = Pos & (MPageSize - 1);
	RdWrCache(true, Handle, NotCached(), PosPg, MPageSize, X);
	wp = (WORD*)(&X[PosI]);
	WORD* wpofs = wp;
	l = *wp;
	if (l <= MPageSize - 2) {       /* small text on 1 page*/
		*wp = -integer(l); N = 0; wp = (WORD*)X;
		while (N < MPageSize - 2) {
			short* signed_wp = (short*)wp;
			if (*signed_wp > 0) {
				FillChar(&X[PosI + 2], l, 0);
				goto label1;
			}
			N += -(*wp) + 2;
			*wpofs += -(*wp) + 2;
		}
		if ((FreePart >= PosPg) && (FreePart < PosPg + MPageSize)) {
			FillChar(X, MPageSize, 0); *XL = -510; FreePart = PosPg;
		label1:
			RdWrCache(false, Handle, NotCached(), PosPg, MPageSize, X);
		}
		else ReleasePage(PosPg);
	}
	else {                        /* long text on more than 1 page */
		if (PosI != 0) goto label3;
	label2:
		l = (WORD)XL;
		if (l > MaxLStrLen + 1) {
		label3:
			Err(889, false); return;
		}
		IsLongTxt = (l = MaxLStrLen + 1); l += 2;
	label4:
		ReleasePage(PosPg);
		if ((l > MPageSize) || IsLongTxt) {
			PosPg = *(longint*)(&X[MPageSize - 4]);
			if ((PosPg < MPageSize) || (PosPg + MPageSize > MLen)) {
				Err(888, false); return;
			}
			RdWrCache(true, Handle, NotCached(), PosPg, MPageSize, X);
			if ((l <= MPageSize)) goto label2; l -= MPageSize - 4; goto label4;
		}
	}
}

LongStr* TFile::Read(WORD StackNr, longint Pos)
{
	LongStr* s = nullptr;
	WORD i = 0, l = 0;
	char* p = nullptr;
	//WORD* pofs = (WORD*)p;
	int offset = 0;
	struct stFptD { longint Typ = 0, Len = 0; } FptD;
	Pos -= LicenseNr;
	if (Pos <= 0 /*OldTxt=-1 in RDB!*/) goto label11;
	else switch (Format) {
	case DbtFormat: {
		s = new LongStr(32768); //(LongStr*)GetStore(32770);
		Pos = Pos << MPageShft;
		p = s->A;
		l = 0;
		while (l <= 32768 - MPageSize) {
			RdWrCache(true, Handle, NotCached(), Pos, MPageSize, &p[offset]);
			for (i = 1; i < MPageSize; i++) { if (p[offset + i] == 0x1A) goto label0; l++; }
			offset += MPageSize;
			Pos += MPageSize;
		}
		l--;
	label0:
		s->LL = l; ReleaseStore(&s->A[l + 1]);
		break;
	}
	case FptFormat: {
		Pos = Pos * BlockSize;
		RdWrCache(true, Handle, NotCached(), Pos, sizeof(FptD), &FptD);
		if (SwapLong(FptD.Typ) != 1/*text*/) goto label11;
		else {
			l = SwapLong(FptD.Len) & 0x7FFF;
			s = new LongStr(l); //(LongStr*)GetStore(l + 2);

			s->LL = l;
			RdWrCache(true, Handle, NotCached(), Pos + sizeof(FptD), l, s->A);
		}
		break;
	}
	default:
		if ((Pos < MPageSize) || (Pos >= MLen)) goto label1;
		RdWrCache(true, Handle, NotCached(), Pos, 2, &l);
		if (l > MaxLStrLen + 1) {
		label1:
			Err(891, false);
		label11:
			if (StackNr == 1) s = new LongStr(l); //(LongStr*)GetStore(2);
			else s = new LongStr(l); //(LongStr*)GetStore2(2);
			s->LL = 0;
			goto label2;
		}
		if (l == MaxLStrLen + 1) { l--; }
		if (StackNr == 1) s = new LongStr(l); //(LongStr*)GetStore(l + 2);
		else s = new LongStr(l + 2); //(LongStr*)GetStore2(l + 2);
		s->LL = l;
		RdWr(true, Pos + 2, l, s->A);
		break;
	}
label2:
	return s;
}

longint TFile::Store(LongStrPtr S)
{
	integer rest; WORD l, M; longint N; void* p; longint pos;
	char X[MPageSize + 1];
	struct stFptD { longint Typ = 0, Len = 0; } FptD;
	longint result = 0;
	l = S->LL;
	if (l == 0) { return result; }
	if (Format == DbtFormat) {
		pos = MaxPage + 1; N = pos << MPageShft; if (l > 0x7fff) l = 0x7fff;
		RdWrCache(false, Handle, NotCached(), N, l, S->A);
		FillChar(X, MPageSize, ' '); X[0] = 0x1A; X[1] = 0x1A;
		rest = MPageSize - (l + 2) % MPageSize;
		RdWrCache(false, Handle, NotCached(), N + l, rest + 2, X);
		MaxPage += (l + 2 + rest) / MPageSize;
		goto label1;
	}
	if (Format == FptFormat) {
		pos = FreePart; N = FreePart * BlockSize;
		if (l > 0x7fff) l = 0x7fff;
		FreePart = FreePart + (sizeof(FptD) + l - 1) / BlockSize + 1;
		FptD.Typ = SwapLong(1); FptD.Len = SwapLong(l);
		RdWrCache(false, Handle, NotCached(), N, sizeof(FptD), &FptD);
		N += sizeof(FptD);
		RdWrCache(false, Handle, NotCached(), N, l, S->A);
		N += l;
		l = FreePart * BlockSize - N;
		if (l > 0) {
			p = GetStore(l); FillChar(p, l, ' ');
			RdWrCache(false, Handle, NotCached(), N, l, p); ReleaseStore(p);
		}
		goto label1;
	}
	if (l > MaxLStrLen) l = MaxLStrLen;
	if (l > MPageSize - 2) pos = NewPage(false);  /* long text */
	else {                                  /* short text */
		rest = MPageSize - FreePart % MPageSize;
		if (l + 2 <= rest) pos = FreePart;
		else { pos = NewPage(false); FreePart = pos; rest = MPageSize; }
		if (l + 4 >= rest) FreePart = NewPage(false);
		else {
			FreePart += l + 2; rest = l + 4 - rest;
			RdWrCache(false, Handle, NotCached(), FreePart, 2, &rest);
		}
	}
	RdWrCache(false, Handle, NotCached(), pos, 2, &l);
	RdWr(false, pos + 2, l, S->A);
label1:
	return pos;
}

void TFile::RdWr(bool ReadOp, longint Pos, WORD N, void* X)
{
	WORD Rest = 0, L = 0;
	longint NxtPg = 0;
	char* source = (char*)X;
	int offset = 0;
	Rest = MPageSize - (WORD(Pos) & (MPageSize - 1));
	while (N > Rest) {
		L = Rest - 4;
		RdWrCache(ReadOp, Handle, NotCached(), Pos, L, &source[offset]);
		offset += L; N -= L;
		if (!ReadOp) NxtPg = NewPage(false);
		RdWrCache(ReadOp, Handle, NotCached(), Pos + L, 4, &NxtPg);
		Pos = NxtPg;
		if (ReadOp && ((Pos < MPageSize) || (Pos + MPageSize > MLen))) {
			Err(890, false);
			FillChar(&source[offset], N, ' ');
			return;
		}
		Rest = MPageSize;
	}
	RdWrCache(ReadOp, Handle, NotCached(), Pos, N, &source[offset]);
}

void TFile::GetMLen()
{
	MLen = (MaxPage + 1) << MPageShft;
}

FileD::FileD()
{
}

FileD::FileD(const FileD& orig)
{
	Name = orig.Name;
	RecLen = orig.RecLen;
	Typ = orig.Typ;
	if (orig.TF != nullptr) TF = new TFile(*orig.TF);
	ChptPos = orig.ChptPos;
	//*OrigFD = orig;
	Drive = orig.Drive;
	if (orig.FldD != nullptr) FldD = new FieldDescr(*orig.FldD);
	IsParFile = orig.IsParFile;
	IsJournal = orig.IsJournal;
	IsHlpFile = orig.IsHlpFile;
	typSQLFile = orig.typSQLFile;
	IsSQLFile = orig.IsSQLFile;
	IsDynFile = orig.IsDynFile;
	if (orig.XF != nullptr) XF = new XFile(*orig.XF);
	if (orig.Keys != nullptr) {
		Keys = new XKey(*orig.Keys, false); // nebudeme kopirovat Keys->KFlds->FldD
		Keys->KFlds->FldD = FldD; // pripojime stavajici FldD
	}
	if (orig.Add != nullptr) Add = new AddD(*orig.Add);
	nLDs = orig.nLDs;
	LiOfs = orig.LiOfs;
}

longint FileD::UsedFileSize()
{
	longint n;
	n = longint(NRecs) * RecLen + FrstDispl;
	if (Typ == 'D') n++;
	return n;
}

bool FileD::IsShared()
{
	return (UMode == Shared) || (UMode == RdShared);
}

bool FileD::NotCached()
{
	if (UMode == Shared) goto label1;
	if (UMode != RdShared) return false;
label1:
	if (LMode == ExclMode) return false;
	return true;
}

bool FileD::Cached()
{
	return !NotCached();
}

WORD FileD::GetNrKeys()
{
	KeyD* k = Keys;
	WORD n = 0;
	while (k != nullptr) { n++; k = k->Chain; }
	return n;
}

LocVar* LocVarBlkD::GetRoot()
{
	if (this->vLocVar.size() > 0) return this->vLocVar[0];
	return nullptr;
}

LocVar* LocVarBlkD::FindByName(std::string Name)
{
	for (auto& i : vLocVar)
	{
		if (SEquUpcase(Name, i->Name)) return i;
	}
	return nullptr;
}


void XString::Clear()
{
	this->S.clean();
}

void XString::StoreReal(double R, KeyFldD* KF)
{
	BYTE A[20];
	// pole urcuje pocet Bytu, ve kterych bude ulozeno cislo
	const BYTE TabF[18] = { 1, 1, 2, 2, 3, 3, 4, 4, 4, 5, 5, 6, 6, 6, 7, 7, 8, 8 };
	auto X = KF->FldD;

	if (X->Typ == 'R' || X->Typ == 'D') {
		bool b = KF->Descend;
		if (R < 0) { b = !b; R = -R; }
		auto r48 = DoubleToReal48(R);
		BYTE date[6];
		for (size_t i = 0; i < 6; i++) {
			date[i] = r48[i];
		}
		StoreD(date, b);
		return;
	}
	if ((X->Flg & f_Comma) == 0) R = R * Power10[X->M];
	WORD n = X->L - 1;
	if (X->M > 0) n--;
	n = TabF[n - 1];
	FixFromReal(R, A, n);
	StoreF(A, n, KF->Descend);
}

void XString::StoreStr(pstring V, KeyFldD* KF)
{
	WORD n = 0;
	auto X = KF->FldD;
	while (V[0] < X->L) {
		if (X->M == LeftJust) V.Append(' ');
		else {
			auto oldV = V;
			V = " ";
			V += oldV;
		}
	}
	if (X->Typ == 'N') {
		for (size_t i = 0; i < X->L; i++) {
			// kolikaty byte zapisujeme?
			size_t iB = i / 2;
			// zapisujeme levou nebo pravou cast?
			if (i % 2 == 0) {
				V[iB + 1] = (V[i + 1] - 0x30) << 4;
			}
			else {
				V[iB + 1] += V[i + 1] - 0x30;
			}
		}
		// Pack(&V[1], &V[0], X->L);
		n = (X->L + 1) / 2;
		V[0] = n;
		StoreN(&V[1], n, KF->Descend);
	}
	else {
		StoreA(&V[1], X->L, KF->CompLex, KF->Descend);
	}
}

void XString::StoreBool(bool B, KeyFldD* KF)
{
	StoreN(&B, 1, KF->Descend);
}

void XString::StoreKF(KeyFldD* KF)
{
	FieldDescr* F = KF->FldD;
	switch (F->FrmlTyp) {
	case 'S': StoreStr(_ShortS(F), KF); break;
	case 'R': StoreReal(_R(F), KF); break;
	case 'B': StoreBool(_B(F), KF); break;
	}
}

void XString::PackKF(KeyFldD* KF)
{
	Clear();
	while (KF != nullptr) {
		StoreKF(KF);
		KF = (KeyFldD*)KF->Chain;
	}
}

bool XString::PackFrml(FrmlList FL, KeyFldD* KF)
{
	FrmlPtr Z;
	Clear();
	while (FL != nullptr) {
		Z = FL->Frml;
		switch (KF->FldD->FrmlTyp) {
		case 'S':StoreStr(RunShortStr(Z), KF); break;
		case 'R':StoreReal(RunReal(Z), KF); break;
		case 'B':StoreBool(RunBool(Z), KF); break;
		}
		KF = (KeyFldD*)KF->Chain;
		FL = (FrmlListEl*)FL->Chain;
	}
	return KF != nullptr;
}

void XString::StoreD(void* R, bool Descend)
{
	unsigned char* data = (unsigned char*)R;
	unsigned char origLen = S[0];
	if (origLen + 6 < origLen) return; // proc to v ASM je? kvuli preteceni?
	S[0] = origLen + 6; // nova delka
	unsigned char D0 = data[0]; // AL
	unsigned char D5 = data[5]; // AH
	unsigned char b5 = (D5 <= 0xF0) ? 0x80 : 0; // nevyssi bit z d5 (AH) - negovany		
	D5 = D5 << 1;
	unsigned char b0 = (D0 & 0x01) << 7; // tento nejnizsi bit z d0 (AL) pujde na nejvyssi v d5
	D0 = (D0 >> 1) + b5; // rcr d0 (AL) + stav neg. CF
	D5 = (D5 >> 1) + b0;

	S[origLen + 1] = D0; // data zacinaji za origLen (jeste je tam 1B delka retezce)
	S[origLen + 2] = D5;

	S[origLen + 3] = data[3];
	S[origLen + 4] = data[4];

	S[origLen + 5] = data[1];
	S[origLen + 6] = data[2];

	if (Descend) {
		// neguj vsechny bity v datech
		for (size_t i = 0; i < 6; i++) {
			S[origLen + 1 + i] = ~S[origLen + 1 + i];
		}
	}
}

void XString::StoreN(void* N, WORD Len, bool Descend)
{
	std::string inpStr((char*)N, Len);
	pstring inpPStr = inpStr;
	S += inpPStr;
}

void XString::StoreF(void* F, WORD Len, bool Descend)
{
	unsigned char* data = (unsigned char*)F;
	unsigned char origLen = S[0];
	if (origLen + Len < Len) return; // proc to v ASM je? kvuli preteceni?
	S[0] = origLen + Len;
	if (data[0] <= 0x0F) S[origLen + 1] = data[0] | 0x80; // 1. bit bude '1'
	else S[origLen + 1] = data[0] & 0x7F; // 1. bit bude '0'
	unsigned char newIndex = origLen + 2; // zacneme zapisovat do S za puvodni data
	// dokopirujeme zbytek dat (1. Byte uz mame, pokracujeme od 2.)
	for (size_t i = 1; i < Len; i++) {
		S[newIndex++] = data[i];
	}
}

void XString::StoreA(void* A, WORD Len, bool CompLex, bool Descend)
{
	int endSpaces = 0; // pocet mezer na konci retezce
	char* p = (char*)A;
	if (CompLex) {
		std::string cplx = TranslateOrd(p);
		Len = cplx.length();
		memcpy(p, cplx.c_str(), Len);
	}
	// nahradi mezery na konci retezce za '0x1F'
	for (int i = Len - 1; i >= 0; i--) {
		if (p[i] == ' ') {
			p[i] = 0x1F;
			endSpaces++;
			continue;
		}
		break;
	}
	// pokud je koncovych mezer vic, kopirujeme jen jednu
	if (endSpaces > 1) {
		Len = Len - (endSpaces - 1);
	}
	auto oldLen = S[0];
	S[0] = oldLen + Len;
	memcpy(&S[oldLen + 1], p, Len);
	if (Descend) {
		// call NegateESDI;
	}
}

XItem::XItem(BYTE* data, bool isLeaf)
{
	Nr = data;
	if (isLeaf) {
		DownPage = (longint*)data;
		XPageData = &data[4];
	}
	else {
		DownPage = (longint*)&data[3];
		XPageData = &data[8];
	}
}

longint XItem::GetN()
{
	return *DownPage & 0x00ffffff;
}

void XItem::PutN(longint N)
{
	// asm les bx,Self; mov ax,N.word; mov es:[bx],ax; mov al,N[2].byte;
	// mov es : [bx + 2] , al

}

WORD XItem::GetM(WORD O)
{
	// asm les bx,Self; add bx,O; xor ah,ah; mov al,es:[bx]
	return 0;
}

void XItem::PutM(WORD O, WORD M)
{
	// asm les bx,Self; add bx,O; mov ax,M; mov es:[bx],al
}

WORD XItem::GetL(WORD O)
{
	// asm les bx,Self; add bx,O; xor ah,ah; mov al,es:[bx+1]
	return 0;
}

void XItem::PutL(WORD O, WORD L)
{
	// asm les bx,Self; add bx,O; mov ax,L; mov es:[bx+1],al
}

XItem* XItem::Next(WORD O, bool isLeaf)
{
	// O by melo byt 3 nebo 7 - urcuje delku "hlavicky" zaznamu
	unsigned char recLen = XPageData[0];
	// dalsi zaznam zacina hned za daty o delce recLen
	auto xi = new XItem(&XPageData[recLen + 1], isLeaf);
	return xi;
}

WORD XItem::UpdStr(WORD O, pstring* S)
{
	/*asm  push ds; lds bx,Self; les di,S; cld; add bx,O;
	 mov al,[bx];{M} add al,[bx+1];{L} stosb;
	 mov al,[bx]; xor ah,ah; add di,ax; lea si,[bx+2];
	 xor ch,ch; mov cl,[bx+1]; rep movsb; mov ax,si; pop ds;*/

	 // 'O' je index
	 // delka S bude M + L
	S[0] = Nr[O] + Nr[O + 1];
	BYTE NrO1 = Nr[O + 1];
	BYTE NrO2 = Nr[O + 2];
	for (BYTE i = 0; i < NrO1; i++) {
		S[NrO1 + i] = Nr[NrO2 + i];
	}

	return NrO2 + NrO1;
}

WORD XPage::Off()
{
	if (IsLeaf) return oLeaf;
	else return oNotLeaf;
}

XItem* XPage::XI(WORD I, bool isLeaf)
{
	_xItem = new XItem(A, isLeaf);
	WORD o = Off();
	while (I > 1) {
		auto oldXitem = _xItem;
		_xItem = _xItem->Next(o, isLeaf);
		delete oldXitem; oldXitem = nullptr;
		I--;
	}
	return _xItem;
}

uintptr_t XPage::EndOff()
{
	XItem* x = XI(NItems + 1, IsLeaf);
	WORD* xofs = (WORD*)x; // absolute x
	return (uintptr_t)xofs;
}

bool XPage::Underflow()
{
	return (EndOff() - uintptr_t(A)) < (XPageSize - XPageOverHead) / 2;
}

bool XPage::Overflow()
{
	return EndOff() - uintptr_t(this) > XPageSize;
}

pstring XPage::StrI(WORD I)
{
	return ""; // netusim, co to ma delat ...
	
	XItem* x = new XItem(A, IsLeaf);
	WORD* xofs = (WORD*)x->Nr[0]; // absolute x
	WORD o = 0;
	pstring s;

	o = Off();
	//TODO: asm les di, @result; mov s.WORD, di; mov s[2].WORD, es;
	pstring es_di;


	if (I > NItems) s[0] = 0;
	else {
		for (WORD j = 1; j <= I; j++) {
			*xofs = x->UpdStr(o, &s);
		}
	}
	//TODO: co a jak to vrací?
	return "";
}

longint XPage::SumN()
{
	if (IsLeaf) { return NItems; }
	longint n = 0;
	XItem* x = new XItem(A, IsLeaf);
	WORD o = Off();
	for (WORD i = 1; i < NItems; i++) {
		n += x->GetN();
		x = x->Next(o, IsLeaf);
	}
	return n;
}

void XPage::Insert(WORD I, void* SS, XItem* XX)
{
	pstring* S = (pstring*)SS;
	XItemPtr x = nullptr, x2 = nullptr;
	WORD* xofs = (WORD*)x;
	WORD* x2ofs = (WORD*)x2;
	WORD m, m2, o, oE, l, l2, sz;
	integer d;

	o = Off(); oE = EndOff();
	NItems++; x = XI(I, IsLeaf);
	m = 0;
	if (I > 1) m = SLeadEqu(StrI(I - 1), *S);
	l = S->length() - m;
	sz = o + 2 + l;
	if (I < NItems) {
		x2 = x;
		m2 = SLeadEqu(StrI(I), *S);
		d = m2 - x->GetM(o);
		if (d > 0) {
			l2 = x->GetL(o); x2ofs += d;
			Move(x, x2, o);
			x2->PutM(o, m2);
			x2->PutL(o, l2 - d);
			sz -= d;
		}
		Move(x2, uintptr_t(x2) + x2ofs + sz, oE - *x2ofs);
	}
	XX = x;
	x->PutM(o, m); x->PutL(o, l);
	xofs += (o + 2);
	Move(&S[m + 1], x, l);
}

void XPage::InsDownIndex(WORD I, longint Page, XPage* P)
{
	pstring s;
	XItemPtr x = nullptr;
	s = P->StrI(P->NItems);
	Insert(I, &s, x);
	x->PutN(P->SumN());
	*(x->DownPage) = Page;
}

void XPage::Delete(WORD I)
{
	XItemPtr x = nullptr, x1 = nullptr, x2 = nullptr;
	WORD* xofs = (WORD*)x;
	WORD* x1ofs = (WORD*)x1;
	WORD* x2ofs = (WORD*)x2;
	WORD o = Off(); WORD oE = EndOff(); x = XI(I, IsLeaf);
	if (I < NItems) {
		x2 = x->Next(o, IsLeaf);
		integer d = x2->GetM(o) - x->GetM(o);
		if (d <= 0) Move(x2, x, oE - *x2ofs);
		else {
			Move(x2, x, o);
			x->PutL(o, x2->GetL(o) + d); x1 = x;
			*x1ofs = *x1ofs + o + 2 + d;
			*x2ofs = *x2ofs + o + 2;
			Move(x2, x1, oE - *x2ofs);
		}
		x = XI(NItems, IsLeaf);
	}
	FillChar(x, oE - *xofs, 0);
	NItems--;
}

void XPage::AddPage(XPage* P)
{
	XItemPtr x = nullptr, x1 = nullptr;
	WORD* xofs = (WORD*)x;

	GreaterPage = P->GreaterPage;
	if (P->NItems == 0) return;
	XItemPtr xE = XI(NItems + 1, IsLeaf);
	WORD oE = P->EndOff(); WORD o = Off(); x = XItemPtr(&P->A);
	if (NItems > 0) {
		WORD m = SLeadEqu(StrI(NItems), P->StrI(1));
		if (m > 0) {
			WORD l = x->GetL(o) - m;
			x1 = x;
			xofs += m;
			Move(x1, x, o);
			x->PutM(o, m); x->PutL(o, l);
		}
	}
	Move(x, xE, oE - *xofs);
	NItems += P->NItems;
}

void XPage::SplitPage(XPage* P, longint ThisPage)
{
	// figuruje tady pstring* s, ale výsledek se nikam neukládá, je to zakomentované

	XItemPtr x = nullptr, x1 = nullptr, x2 = nullptr;
	WORD* xofs = (WORD*)x;
	WORD* x1ofs = (WORD*)x1;
	WORD* x2ofs = (WORD*)x2;
	WORD o, oA, oE, n;
	pstring* s;

	x = XItemPtr(&A); x1 = x; o = Off(); oA = *xofs; oE = EndOff(); n = 0;
	while (*xofs - oA < oE - *xofs + x->GetM(o)) { x = x->Next(o, IsLeaf); n++; }
	FillChar(P, XPageSize, 0);
	Move(x1, P->A, *xofs - oA);
	//s = (pstring*)(uintptr_t(x1) + oA + o + 1);;
	//s = &StrI(n + 1);
	Move(x, x1, o);
	x1->PutM(o, 0);
	x1 = x1->Next(o, IsLeaf); x = x->Next(o, IsLeaf); Move(x, x1, oE - *xofs);
	P->NItems = n; NItems -= n; *xofs = EndOff(); FillChar(x, oE - *xofs, 0);
	if (IsLeaf) P->GreaterPage = ThisPage; else P->GreaterPage = 0;
	P->IsLeaf = IsLeaf;
}

void XPage::Clean()
{
	IsLeaf = false;
	GreaterPage = 0;
	WORD NItems = 0;
	memset(A, 0, XPageSize - 4);
}

XKey::XKey()
{
}

XKey::XKey(const XKey& orig, bool copyFlds)
{
	if (orig.Chain != nullptr) Chain = new XKey(*orig.Chain);
	if (orig.KFlds != nullptr) KFlds = new KeyFldD(*orig.KFlds, copyFlds);
	Intervaltest = orig.Intervaltest;
	Duplic = orig.Duplic;
	InWork = orig.InWork;
	IndexRoot = orig.IndexRoot;
	IndexLen = orig.IndexLen;
	NR = orig.NR;
	Alias = orig.Alias;
}

XKey::XKey(BYTE* inputStr)
{
	size_t index = 0;
	Chain = reinterpret_cast<XKey*>(*(unsigned int*)&inputStr[index]); index += 4;
	KFlds = reinterpret_cast<KeyFldD*>(*(unsigned int*)&inputStr[index]); index += 4;
	Intervaltest = *(bool*)&inputStr[index]; index++;
	Duplic = *(bool*)&inputStr[index]; index++;
	InWork = *(bool*)&inputStr[index]; index++;
	IndexRoot = *(unsigned short*)&inputStr[index]; index += 2;
	IndexLen = *(unsigned char*)&inputStr[index]; index++;
	NR = *(longint*)&inputStr[index]; index += 4;
	Alias = reinterpret_cast<pstring*>(*(unsigned int*)&inputStr[index]); index += 4;

	//unsigned int DisplOrFrml = *(unsigned int*)&inputStr[index]; index += 4;
	//if (DisplOrFrml > MaxTxtCols) {
	//	// jedna se o ukazatel
	//	Frml = reinterpret_cast<FrmlElem*>(DisplOrFrml);
	//}
	//else {
	//	// jedna se o delku
	//	Displ = DisplOrFrml;
	//}
	//Name[0] = inputStr[index]; index++;
	//memcpy(&Name[1], &inputStr[index], Name[0]); index += Name[0];
}

XWFile* XKey::XF()
{
	if (InWork) return &XWork;
	return CFile->XF;
}

longint XKey::NRecs()
{
	if (InWork) return NR;
	return CFile->XF->NRecs;
}

bool XKey::Search(XString& XX, bool AfterEqu, longint& RecNr)
{
	bool searchResult = false;
	XPage* p = nullptr;
	WORD iItem = 0;
	XItem* x = nullptr;
	size_t iItemIndex = 0;
	char result = '\0';
	p = new XPage(); // (XPage*)GetStore(XPageSize);
	XPathN = 1;
	longint page = IndexRoot;
	AfterEqu = AfterEqu && Duplic;
label1:
	XPath[XPathN].Page = page;
	XF()->RdPage(p, page); // je nactena asi cela stranka indexu

	WORD o = p->Off();
	WORD nItems = p->NItems;
	if (nItems == 0) {
		RecNr = CFile->NRecs + 1;
		XPath[1].I = 1;
		goto label2;
	}

	// * PUVODNI ASM
	result = XKeySearch2(p->A, &XX.S[0], iItem, iItemIndex, nItems, o, AfterEqu);
	// * KONEC PUVODNIHO ASM

	XPath[XPathN].I = iItem;
	x = new XItem(&p->A[iItemIndex], p->IsLeaf);
	if (p->IsLeaf) {
		if (iItem > nItems) RecNr = CFile->NRecs + 1;
		else RecNr = x->GetN();
		if (result == _equ)
			if
#ifdef FandSQL
				!CFile->IsSQLFile&&
#endif
				(((RecNr == 0) || (RecNr > CFile->NRecs))) XF()->Err(833);
			else searchResult = true;
		else
			label2:
		searchResult = false;
		ReleaseStore(p);
		delete x;
		return searchResult;
	}
	if (iItem > nItems) page = p->GreaterPage;
	else page = *(x->DownPage);
	XPathN++;
	delete x;
	goto label1;
}

bool XKey::SearchIntvl(XString& XX, bool AfterEqu, longint& RecNr)
{
	return Search(XX, AfterEqu, RecNr) || Intervaltest && (RecNr <= CFile->NRecs);
}

longint XKey::PathToNr()
{
	longint n = 0;
	XPage* p = new XPage(); // (XPage*)GetStore(XPageSize);
	for (WORD j = 1; j <= XPathN - 1; j++)
	{
		XF()->RdPage(p, XPath[j].Page);
		XItem* x = new XItem(p->A, p->IsLeaf); // XItemPtr(p->A);
		for (WORD i = 1; i <= XPath[j].I - 1; i++) {
			n += x->GetN();
			auto prevX = x;
			x = x->Next(oNotLeaf, p->IsLeaf);
			delete prevX; prevX = 0;
		}
		delete x; x = nullptr;
	}
	n += XPath[XPathN].I;
	if (n > NRecs() + 1) XF()->Err(834);
	ReleaseStore(p);
	return n;
}

void XKey::NrToPath(longint I)
{
	XPage* p = new XPage(); // (XPage*)GetStore(XPageSize);
	longint page = IndexRoot;
	XPathN = 0;
label1:
	XF()->RdPage(p, page);
	XPathN++;
	XPath[XPathN].Page = page;
	if (p->IsLeaf) {
		if (I > p->NItems + 1) XF()->Err(837);
		XPath[XPathN].I = I;
		ReleaseStore(p);
		return;
	}
	XItem* x = new XItem(p->A, p->IsLeaf);
	for (WORD j = 1; j <= p->NItems; j++) {
		if (I <= x->GetN()) {
			XPath[XPathN].I = j;
			page = *(x->DownPage);
			delete x; x = nullptr;
			goto label1;
		}
		I -= x->GetN();
		auto prevX = x;
		x = x->Next(oNotLeaf, p->IsLeaf);
		delete prevX; prevX = nullptr;
	}
	delete x; x = nullptr;
	XPath[XPathN].I = p->NItems + 1;
	page = p->GreaterPage;
	goto label1;
}

longint XKey::PathToRecNr()
{
	/* !!! with XPath[XPathN] do!!! */
	auto X = XPath[XPathN];
	XPage* p = new XPage(); // (XPage*)GetStore(XPageSize);
	XF()->RdPage(p, X.Page);
	auto pxi = p->XI(X.I, p->IsLeaf);
	longint recnr = pxi->GetN();
	longint result = recnr;
	if ((recnr == 0) || (recnr > CFile->NRecs)) XF()->Err(835);
	ReleaseStore(p);
	return result;
}

bool XKey::RecNrToPath(XString& XX, longint RecNr)
{
	bool result = false;
	XItem* x = nullptr; longint n = 0;
	XX.PackKF(KFlds);
	Search(XX, false, n);
	XPage* p = new XPage(); // (XPage*)GetStore(XPageSize);
	/* !!! with XPath[XPathN] do!!! */
	{
		auto X = XPath[XPathN];
	label1:
		XF()->RdPage(p, X.Page);
		x = p->XI(X.I, p->IsLeaf);
		if (!(p->StrI(X.I) == XX.S)) goto label3;
	label2:
		if (x->GetN() == RecNr) { result = true; goto label3; }
		X.I++;
		if (X.I > p->NItems) {
			if (IncPath(XPathN - 1, X.Page)) { X.I = 1; goto label1; }
		}
		else {
			x = x->Next(oLeaf, p->IsLeaf);
			if (x->GetL(oLeaf) != 0) goto label3;
			goto label2;
		}
	}
label3:
	ReleaseStore(p);
	return result;
}

bool XKey::IncPath(WORD J, longint& Pg)
{
	XPage* p = new XPage(); // (XPage*)GetStore(XPageSize);
	bool result = false;
	auto X = XPath[J];
	if (J == 0) { goto label2; } /* !!! with XPath[J] do!!! */
	{
	label1:
		XF()->RdPage(p, X.Page);
		if (X.I > p->NItems)
			if (IncPath(J - 1, X.Page)) { X.I = 0; goto label1; }
			else goto label2;
		X.I++;
		if (X.I > p->NItems)
			if (p->GreaterPage == 0) {
				X.I = 0; if (IncPath(J - 1, X.Page)) goto label1; goto label2;
			}
			else Pg = p->GreaterPage;
		else Pg = *(p->XI(X.I, p->IsLeaf)->DownPage);
	}
	result = true;
label2:
	ReleaseStore(p);
	return result;
}

longint XKey::NrToRecNr(longint I)
{
	NrToPath(I);
	return PathToRecNr();
}

pstring XKey::NrToStr(longint I)
{
	pstring result;
	XPage* p = new XPage(); // (XPage*)GetStore(XPageSize);
	NrToPath(I);
	/* !!! with XPath[XPathN] do!!! */
	XF()->RdPage(p, XPath[XPathN].Page);
	result = p->StrI(I);
	ReleaseStore(p);
	return result;
}

longint XKey::RecNrToNr(longint RecNr)
{
	XString x;
	if (RecNrToPath(x, RecNr)) return PathToNr();
	else return 0;
}

bool XKey::FindNr(XString& X, longint& IndexNr)
{
	longint n;
	auto result = Search(X, false, n);
	IndexNr = PathToNr();
	return result;
}

void XKey::InsertOnPath(XString& XX, longint RecNr)
{
	WORD i = 0, j = 0;
	longint page = 0, page1 = 0, uppage = 0, downpage = 0;
	XItem* x = nullptr;
	longint n = 0, upsum = 0;

	XPage* p = new XPage(); // (XPage*)GetStore(2 * XPageSize);
	XPage* p1 = new XPage(); // (XPage*)GetStore(2 * XPageSize);
	XPage* upp = new XPage(); // (XPage*)GetStore(2 * XPageSize);
	for (j = XPathN; j > 1; j--) {
		page = XPath[j].Page;
		XF()->RdPage(p, page);
		i = XPath[j].I;
		if (p->IsLeaf) {
			InsertItem(XX, p, upp, page, i, x, uppage);
			x->PutN(RecNr);
		}
		else {
			if (i <= p->NItems) {
				x = p->XI(i, p->IsLeaf);
				n = x->GetN() + 1;
				if (uppage != 0) n -= upsum;
				x->PutN(n);
			}
			if (uppage != 0) {
				downpage = uppage;
				InsertItem(XX, p, upp, page, i, x, uppage);
				*(x->DownPage) = downpage;
				x->PutN(upsum);
			}
		}
		XF()->WrPage(p, page);
		if (uppage != 0) {
			XF()->WrPage(upp, uppage);
			upsum = upp->SumN();
			if (upp->IsLeaf) ChainPrevLeaf(p1, uppage);
		}
	}
	if (uppage != 0) {
		page1 = XF()->NewPage(p1);
		p1->GreaterPage = page1;
		p1->InsDownIndex(1, uppage, upp);
		XF()->WrPage(p, page1);
		XF()->WrPage(p1, page);
		if (upp->IsLeaf) {
			upp->GreaterPage = page1;
			XF()->WrPage(upp, uppage);
		}
	}
	ReleaseStore(p);
}

void XKey::InsertItem(XString& XX, XPage* P, XPage* UpP, longint Page, WORD I, XItemPtr& X, longint& UpPage)
{
	P->Insert(I, &XX.S, X);
	UpPage = 0;
	if (P->Overflow()) {
		UpPage = XF()->NewPage(UpP);
		P->SplitPage(UpP, Page);
		if (I <= UpP->NItems) X = UpP->XI(I, P->IsLeaf);
		else X = P->XI(I - UpP->NItems, P->IsLeaf);
		XX.S = UpP->StrI(UpP->NItems);
	}
}

void XKey::ChainPrevLeaf(XPagePtr P, longint N)
{
	longint page = 0;
	WORD i = 0, j = 0;
	for (j = XPathN - 1; j > 1; j--)
		if (XPath[j].I > 1) {
			XF()->RdPage(P, XPath[j].Page);
			i = XPath[j].I - 1;
		label1:
			page = *(P->XI(i, P->IsLeaf)->DownPage);
			XF()->RdPage(P, page);
			if (P->IsLeaf) {
				P->GreaterPage = N;
				XF()->WrPage(P, page);
				return;
			}
			i = P->NItems;
			goto label1;
		}
}

bool XKey::Insert(longint RecNr, bool Try)
{
	longint N = 0, XNr = 0; XString x;
	x.PackKF(KFlds);
	if (Search(x, true, N)) {
		if (Try) { return false; }
	}
	else
	{
		XFNotValid();
		CFileError(822);
	}
	InsertOnPath(x, RecNr);
	return true;
}

void XKey::DeleteOnPath()
{
	longint page, page1, page2;
	longint uppage = 0;
	void* pp = nullptr;
	XItem* x = nullptr;
	bool released;
	longint n;

	MarkStore(pp);
	XPage* p = (XPage*)GetStore(2 * XPageSize);
	XPage* p1 = (XPage*)GetStore(2 * XPageSize);
	XPage* p2 = (XPage*)GetStore(2 * XPageSize);
	XPage* upp = p2;
	for (WORD j = XPathN; j > 1; j--) {
		page = XPath[j].Page; XF()->RdPage(p, page); WORD i = XPath[j].I;
		if (p->IsLeaf) p->Delete(i);
		else if (upp->Underflow()) {
			XF()->WrPage(upp, uppage);
			WORD i1 = i - 1;
			WORD i2 = i;
			if (i1 == 0) { i1 = 1; i2 = 2; }
			XIDown(p, p1, i1, page1); XIDown(p, p2, i2, page2);
			BalancePages(p1, p2, released);
			XF()->WrPage(p1, page1); p->Delete(i1);
			if (released) {
				XF()->ReleasePage(p2, page2);
				if (i1 > p->NItems) p->GreaterPage = page1;
				else { p->InsDownIndex(i1, page1, p1); p->Delete(i2); }
			}
			else {
				XF()->WrPage(p2, page2); p->InsDownIndex(i1, page1, p1);
				if (i2 <= p->NItems) {
					p->Delete(i2); p->InsDownIndex(i2, page2, p2);
				}
			}
		}
		else {
			if (upp->Overflow()) {
				page1 = XF()->NewPage(p1); upp->SplitPage(p1, uppage);
				XF()->WrPage(p1, page1); p->InsDownIndex(i, page1, p1); i++;
			}
			XF()->WrPage(upp, uppage);
			if (i <= p->NItems) {
				p->Delete(i); p->InsDownIndex(i, uppage, upp);
			}
		}
		uppage = page;
		XPage* px = upp;
		upp = p;
		p = px;
	}
	if (upp->Overflow()) {
		page1 = XF()->NewPage(p1); upp->SplitPage(p1, uppage); page = XF()->NewPage(p);
		p->GreaterPage = page; p->InsDownIndex(1, page1, p1);
		XF()->WrPage(p1, page1); XF()->WrPage(p, uppage); XF()->WrPage(upp, page);
	}
	else {
		page1 = upp->GreaterPage;
		if ((upp->NItems == 0) && (page1 > 0)) {
			XF()->RdPage(p1, page1); Move(p1, upp, XPageSize);
			XF()->ReleasePage(p1, page1);
		}
		XF()->WrPage(upp, uppage);
	}
	ReleaseStore(pp);
}

void XKey::BalancePages(XPage* P1, XPage* P2, bool& Released)
{
	longint n; WORD sz;
	n = P1->GreaterPage;
	P1->AddPage(P2);
	sz = P1->EndOff() - uintptr_t(P1);
	if (sz <= XPageSize) Released = true;
	else {
		Released = false; Move(P1, P2, sz);
		P2->SplitPage(P1, n);
	}
}

void XKey::XIDown(XPage* P, XPage* P1, WORD I, longint& Page1)
{
	if (I > P->NItems) Page1 = P->GreaterPage;
	else Page1 = *(P->XI(I, P->IsLeaf)->DownPage);
	XF()->RdPage(P1, Page1);
}

bool XKey::Delete(longint RecNr)
{
	XString xx;
	bool b = RecNrToPath(xx, RecNr);
	if (b) DeleteOnPath();
	return b;
}

void XWKey::Open(KeyFldD* KF, bool Dupl, bool Intvl)
{
	KFlds = KF;
	Duplic = Dupl;
	InWork = true;
	Intervaltest = Intvl;
	NR = 0;
	//XPage* p = (XPage*)GetStore(sizeof(p)); 
	XPage* p = new XPage();
	IndexRoot = XF()->NewPage(p);
	p->IsLeaf = true;
	XF()->WrPage(p, IndexRoot);
	ReleaseStore(p);
	IndexLen = 0;
	while (KF != nullptr) {
		if (KF->FldD != nullptr) IndexLen += KF->FldD->NBytes;
		KF = (KeyFldD*)KF->Chain;
	}
}

void XWKey::Close()
{
	ReleaseTree(IndexRoot, true);
	IndexRoot = 0;
}

void XWKey::Release()
{
	ReleaseTree(IndexRoot, false);
	NR = 0;
}

void XWKey::ReleaseTree(longint Page, bool IsClose)
{
	if ((Page == 0) || (Page > XF()->MaxPage)) return;
	XPage* p = new XPage(); // (XPage*)GetStore(XPageSize);
	XF()->RdPage(p, Page);
	if (!p->IsLeaf) {
		WORD n = p->NItems;
		for (WORD i = 1; i <= n; i++) {
			ReleaseTree(*(p->XI(i, p->IsLeaf)->DownPage), IsClose);
			XF()->RdPage(p, Page);
		}
		if (p->GreaterPage != 0) ReleaseTree(p->GreaterPage, IsClose);
	}
	if ((Page != IndexRoot) || IsClose)
		XF()->ReleasePage(p, Page);
	else {
		FillChar(p, XPageSize, 0);
		p->IsLeaf = true;
		XF()->WrPage(p, Page);
	}
	ReleaseStore(p);
}

void XWKey::OneRecIdx(KeyFldD* KF, longint N)
{
	Open(KF, true, false);
	Insert(N, true);
	NR++;
}

void XWKey::InsertAtNr(longint I, longint RecNr)
{
	XString x;
	x.PackKF(KFlds);
	NR++;
	NrToPath(I);
	InsertOnPath(x, RecNr);
}

longint XWKey::InsertGetNr(longint RecNr)
{
	XString x; longint n;
	NR++; x.PackKF(KFlds);
	Search(x, true, n);
	auto result = PathToNr();
	InsertOnPath(x, RecNr);
	return result;
}

void XWKey::DeleteAtNr(longint I)
{
	NrToPath(I);
	DeleteOnPath();
	NR--;
}

void XWKey::AddToRecNr(longint RecNr, integer Dif)
{
	if (NRecs() == 0) return;
	NrToPath(1);
	XPage* p = new XPage(); // (XPage*)GetStore(sizeof(*p));
	/* !!! with XPath[XPathN] do!!! */
	longint pg = XPath[XPathN].Page;
	integer j = XPath[XPathN].I;
	do {
		XF()->RdPage(p, pg);
		integer n = p->NItems - j + 1;
		XItem* x = p->XI(j, p->IsLeaf);
		while (n > 0) {
			longint nn = x->GetN();
			if (nn >= RecNr) x->PutN(nn + Dif);
			auto prevX = x;
			x = x->Next(oLeaf, p->IsLeaf);
			delete prevX; prevX = nullptr;
			n--;
		}
		XF()->WrPage(p, pg);
		pg = p->GreaterPage;
		j = 1;
	} while (pg != 0);
	ReleaseStore(p);
}

void XWFile::Err(WORD N)
{
	if (this == &XWork) {
		SetMsgPar(FandWorkXName);
		RunError(N);
	}
	else {
		CFile->XF->SetNotValid();
		CFileMsg(N, 'X');
		CloseGoExit();
	}
}

void XWFile::TestErr()
{
	if (HandleError != 0) Err(700 + HandleError);
}

longint XWFile::UsedFileSize()
{
	return longint(MaxPage + 1) << XPageShft;
}

bool XWFile::NotCached()
{
	return (this != &XWork) && CFile->NotCached();
}

void XWFile::RdPage(XPage* P, longint N)
{
	if ((N == 0) || (N > MaxPage)) Err(831);
	// puvodne se nacitalo celych XPageSize z P, bylo nutno to rozhodit na jednotlive tridni promenne
	RdWrCache(true, Handle, NotCached(), N << XPageShft, 1, &P->IsLeaf);
	RdWrCache(true, Handle, NotCached(), (N << XPageShft) + 1, 4, &P->GreaterPage);
	RdWrCache(true, Handle, NotCached(), (N << XPageShft) + 5, 2, &P->NItems);
	RdWrCache(true, Handle, NotCached(), (N << XPageShft) + 7, XPageSize - 7, P->A);
}

void XWFile::WrPage(XPage* P, longint N)
{
	if (UpdLockCnt > 0) Err(645);
	// puvodne se zapisovalo celych XPageSize z P, bylo nutno to rozhodit na jednotlive tridni promenne
	RdWrCache(false, Handle, NotCached(), N << XPageShft, 1, &P->IsLeaf);
	RdWrCache(false, Handle, NotCached(), (N << XPageShft) + 1, 4, &P->GreaterPage);
	RdWrCache(false, Handle, NotCached(), (N << XPageShft) + 5, 2, &P->NItems);
	RdWrCache(false, Handle, NotCached(), (N << XPageShft) + 7, XPageSize - 7, P->A);
}

longint XWFile::NewPage(XPage* P)
{
	longint result = 0;
	if (FreeRoot != 0) {
		result = FreeRoot;
		RdPage(P, FreeRoot);
		FreeRoot = P->GreaterPage;
	}
	else {
		MaxPage++;
		if (MaxPage > 0x1fffff) Err(887);
		result = MaxPage;
	}
	P->IsLeaf = false;
	P->GreaterPage = 0;
	P->NItems = 0;
	memset(P->A, 0, XPageSize - 4);
	return result;
}

void XWFile::ReleasePage(XPage* P, longint N)
{
	P->IsLeaf = false;
	P->NItems = 0;
	//FillChar(P, XPageSize, 0);
	memset(P->A, 0, sizeof(P->A));
	P->GreaterPage = FreeRoot;
	FreeRoot = N;
	WrPage(P, N);
}

XFile::XFile(const XFile& orig)
{
	NRecs = orig.NRecs;
	NRecsAbs = orig.NRecsAbs;
	NotValid = orig.NotValid;
	NrKeys = orig.NrKeys;
	NoCreate = orig.NoCreate;
	FirstDupl = orig.FirstDupl;
}

void XFile::SetEmpty()
{
	auto p = new XPage(); // (XPage*)GetZStore(XPageSize);
	WrPage(p, 0);
	p->IsLeaf = true;
	FreeRoot = 0;
	NRecs = 0;
	KeyD* k = CFile->Keys;
	while (k != nullptr) {
		longint n = k->IndexRoot;
		MaxPage = n;
		WrPage(p, n);
		k = k->Chain;
	}
	ReleaseStore(p);
	WrPrefix();
}

void XFile::RdPrefix()
{
	RdWrCache(true, Handle, NotCached(), 2, 4, &FreeRoot);
	RdWrCache(true, Handle, NotCached(), 6, 4, &MaxPage);
	RdWrCache(true, Handle, NotCached(), 10, 4, &NRecs);
	RdWrCache(true, Handle, NotCached(), 14, 4, &NRecsAbs);
	RdWrCache(true, Handle, NotCached(), 18, 1, &NotValid);
	RdWrCache(true, Handle, NotCached(), 19, 1, &NrKeys);
}

void XFile::WrPrefix()
{
	WORD Signum = 0x04FF;
	RdWrCache(false, Handle, NotCached(), 0, 2, &Signum);
	NRecsAbs = CFile->NRecs;
	NrKeys = CFile->GetNrKeys();
	RdWrCache(false, Handle, NotCached(), 2, 4, &FreeRoot);
	RdWrCache(false, Handle, NotCached(), 6, 4, &MaxPage);
	RdWrCache(false, Handle, NotCached(), 10, 4, &NRecs);
	RdWrCache(false, Handle, NotCached(), 14, 4, &NRecsAbs);
	RdWrCache(false, Handle, NotCached(), 18, 1, &NotValid);
	RdWrCache(false, Handle, NotCached(), 19, 1, &NrKeys);
}

void XFile::SetNotValid()
{
	NotValid = true;
	MaxPage = 0;
	WrPrefix();
	SaveCache(0, CFile->Handle);
}

XScan::XScan(FileD* aFD, KeyD* aKey, KeyInD* aKIRoot, bool aWithT)
{
	FD = aFD; Key = aKey; KIRoot = aKIRoot; withT = aWithT;
#ifdef FandSQL
	if (aFD->IsSQLFile) {
		if ((aKey != nullptr) && aKey->InWork) { P = (XPage*)GetStore(XPageSize); Kind = 3; }
		else Kind = 4;
	}
	else
#endif
	{
		if (aKey != nullptr) {
			//P = (XPage*)GetStore(XPageSize);
			P = new XPage();
			Kind = 1;
			if (aKIRoot != nullptr) Kind = 2;
		}
	}
}

void XScan::Reset(FrmlElem* ABool, bool SQLFilter)
{
	KeyInD* k = nullptr; longint n = 0; XString xx; bool b = false;
	CFile = FD;
	Bool = ABool;
	if (SQLFilter) {
		if (CFile->IsSQLFile) hasSQLFilter = true;
		else Bool = nullptr;
	}
	switch (Kind) {
	case 0: NRecs = CFile->NRecs; break;
	case 1:
	case 3: {
		if (Key != nullptr) {
			if (!Key->InWork) TestXFExist();
			NRecs = Key->NRecs();
		}
		break;
	}
	case 2: {
		if (!Key->InWork) TestXFExist();
		CompKIFrml(Key, KIRoot, true);
		NRecs = 0;
		k = KIRoot;
		while (k != nullptr) {
			XString a1;
			XString b2;
			a1.S = k->X1;
			b2.S = k->X2;
			// vyhleda 1. zaznam odpovidajici klici 
			Key->FindNr(a1, k->XNrBeg);
			// vyhleda posledni zaznam odpovidajici klici
			b = Key->FindNr(b2, n);
			k->N = 0;
			if (n >= k->XNrBeg) k->N = n - k->XNrBeg + b;
			NRecs += k->N;
			k = (KeyInD*)k->Chain;
		}
		break;
	}
#ifdef FandSQL
	case 4: { CompKIFrml(Key, KIRoot, false); New(SQLStreamPtr(Strm), Init); IRec = 1; break; }
#endif
	}
	SeekRec(0);
}

void XScan::ResetSort(KeyFldD* aSK, FrmlPtr& BoolZ, LockMode OldMd, bool SQLFilter)
{
	LockMode m;
	if (Kind == 4) {
		SK = aSK;
		if (SQLFilter)
		{
			Reset(BoolZ, true);
			BoolZ = nullptr;
		}
		else Reset(nullptr, false);
		return;
	}
	if (aSK != nullptr) {
		Reset(BoolZ, false);
		ScanSubstWIndex(this, aSK, 'S');
		BoolZ = nullptr;
	}
	else Reset(nullptr, false);
	/* !!! with CFile^ do!!! */
	if (CFile->NotCached()) {
		switch (Kind) {
		case 0: { m = NoCrMode; if (CFile->XF != nullptr) m = NoExclMode; break; }
		case 1: { m = OldMd; if (Key->InWork) m = NoExclMode; break; }
		default: return;
		}
		m = LockMode(MaxW(m, OldMd));
		if (m != OldMd) ChangeLMode(m, 0, true);
	}
}

void XScan::SubstWIndex(WKeyDPtr WK)
{
	Key = WK;
	if (Kind != 3) Kind = 1;
	if (P == nullptr) P = new XPage(); // (XPage*)GetStore(XPageSize);
	NRecs = Key->NRecs();
	Bool = nullptr;
	SeekRec(0);
	TempWX = true;
}

void XScan::ResetOwner(XString* XX, FrmlPtr aBool)
{
	longint n;
	bool b;
	CFile = FD;
	Bool = aBool;
#ifdef FandSQL
	if (Kind = 4) {           /* !on .SQL with Workindex */
		KIRoot = GetZStore(sizeof(KIRoot^));
		KIRoot->X1 = StoreStr(XX->S); KIRoot->X2 = StoreStr(XX->S);
		New(SQLStreamPtr(Strm), Init); IRec = 1
	}
	else
#endif
	{
		TestXFExist();
		KIRoot = new KeyInD(); // (KeyInD*)GetZStore(sizeof(*KIRoot));
		Key->FindNr(*XX, KIRoot->XNrBeg);
		AddFFs(Key, XX->S);
		b = Key->FindNr(*XX, n);
		NRecs = n - KIRoot->XNrBeg + b;
		KIRoot->N = NRecs; Kind = 2;
	}
	SeekRec(0);
}

bool EquKFlds(KeyFldD* KF1, KeyFldD* KF2)
{
	bool result = false;
	while (KF1 != nullptr) {
		if ((KF2 == nullptr) || (KF1->CompLex != KF2->CompLex) || (KF1->Descend != KF2->Descend)
			|| (KF1->FldD->Name != KF2->FldD->Name)) return result;
		KF1 = (KeyFldD*)KF1->Chain;
		KF2 = (KeyFldD*)KF2->Chain;
	}
	if (KF2 != nullptr) return false;
	return true;
}

void XScan::ResetOwnerIndex(LinkDPtr LD, LocVar* LV, FrmlPtr aBool)
{
	WKeyDPtr k;
	CFile = FD; TestXFExist(); Bool = aBool; OwnerLV = LV; Kind = 2;
	if (!EquKFlds(WKeyDPtr(LV->RecPtr)->KFlds, LD->ToKey->KFlds)) RunError(1181);
	SeekRec(0);
}

#ifdef FandSQL
void XScan::ResetSQLTxt(FrmlPtr Z)
{
	LongStrPtr s;
	New(SQLStreamPtr(Strm), Init); s = RunLongStr(Z);
	SQLStreamPtr(Strm)->InpResetTxt(s); ReleaseStore(s);
	eof = false;
}
#endif

void XScan::ResetLV(void* aRP)
{
	Strm = aRP; Kind = 5; NRecs = 1;
}

void XScan::Close()
{
	CFile = FD;
#ifdef FandSQL
	if (Kind = 4) /* !!! with SQLStreamPtr(Strm)^ do!!! */ { InpClose; Done; }
#endif
	if (TempWX) WKeyDPtr(Key)->Close();
}

void XScan::SeekRec(longint I)
{
	KeyInD* k = nullptr;
	FrmlElem* z = nullptr;
	CFile = FD;

#ifdef FandSQL
	if (Kind == 4) {
		if (I != IRec) /* !!! with SQLStreamPtr(Strm)^ do!!! */
		{
			if (NotFrst) InpClose; NotFrst = true;
			if (hasSQLFilter) z = Bool else z = nullptr;
			InpReset(Key, SK, KIRoot, z, withT);
			EOF = AtEnd; IRec = 0; NRecs = 0x20000000;
		}
		return;
	}
#endif
	if ((Kind == 2) && (OwnerLV != nullptr)) {
		IRec = 0;
		NRecs = 0x20000000;
		iOKey = 0;
		NextIntvl();
		eof = I >= NRecs;
		return;
	}
	IRec = I;
	eof = I >= NRecs;
	if (!eof) {
		switch (Kind) {
		case 1:
		case 3: {
			Key->NrToPath(I + 1); /* !!! with XPath[XPathN] do!!! */
			SeekOnPage(XPath[XPathN].Page, XPath[XPathN].I);
			break;
		}
		case 2: {
			k = KIRoot;
			while (I >= k->N) { I -= k->N; k = (KeyInD*)k->Chain; }
			KI = k;
			SeekOnKI(I);
			break;
		}
		}
	}
}

bool DeletedFlag()  // r771 ASM
{
	if (CFile->Typ == 'X') {
		if (((BYTE*)CRecPtr)[0] == 0) return false;
		return true;
	}

	if (CFile->Typ == 'D') {
		if (((BYTE*)CRecPtr)[0] != '*') return false;
		return true;
	}

	return false;
}

void ClearDeletedFlag()
{
	switch (CFile->Typ) {
	case 'X': ((BYTE*)CRecPtr)[0] = 0; break;
	case 'D': ((BYTE*)CRecPtr)[0] = ' '; break;
	}
}

void SetDeletedFlag()
{
	switch (CFile->Typ) {
	case 'X': ((BYTE*)CRecPtr)[0] = 1; break;
	case 'D': ((BYTE*)CRecPtr)[0] = '*'; break;
	}
}

integer CompStr(pstring& S1, pstring& S2)
{
	return 0;
}

WORD CmpLxStr(char* p1, WORD len1, char* p2, WORD len2)
{
	if (len1 > 0) {
		for (size_t i = len1 - 1; i > 0; i--) {
			if (p1[i] == ' ') { len1--; continue; }
			break;
		}
	}
	if (len2 > 0) {
		for (size_t i = len2 - 1; i > 0; i--) {
			if (p2[i] == ' ') { len1--; continue; }
			break;
		}
	}

	WORD cmpLen = min(len1, len2);
	for (size_t i = 0; i < cmpLen; i++) {
		if (p1[i] == p2[i]) continue;
		if (p1[i] < p2[i]) return _lt;
		return _gt;
	}
	if (len1 < len2) return _lt;
	if (len1 > len2) return _gt;
	return _equ;
}

WORD CompLexLongStr(LongStrPtr S1, LongStrPtr S2)
{
	WORD l1 = min(S1->LL, 256);
	char* b1 = new char[l1];
	WORD l2 = min(S2->LL, 256);
	char* b2 = new char[l2];

	for (size_t i = 0; i < l1; i++) {
		b1[i] = CharOrdTab[S1->A[i]];
	}
	for (size_t i = 0; i < l2; i++) {
		b2[i] = CharOrdTab[S2->A[i]];
	}

	auto result = CmpLxStr(b1, l1, b2, l2);

	delete[] b1;
	delete[] b2;
	return result;
}

WORD CompLexLongShortStr(LongStrPtr S1, pstring& S2)
{
	WORD l1 = min(S1->LL, 256);
	char* b1 = new char[l1];
	WORD l2 = S2[0];
	char* b2 = new char[l2];

	for (size_t i = 0; i < l1; i++) {
		b1[i] = CharOrdTab[S1->A[i]];
	}
	for (size_t i = 0; i < l2; i++) {
		b2[i] = CharOrdTab[S2[i + 1]];
	}

	auto result = CmpLxStr(b1, l1, b2, l2);

	delete[] b1;
	delete[] b2;
	return result;
}

WORD CompLexStr(pstring& S1, pstring& S2)
{
	WORD l1 = S1[0];
	char* b1 = new char[l1];
	WORD l2 = S2[0];
	char* b2 = new char[l2];

	for (size_t i = 0; i < l1; i++) {
		b1[i] = CharOrdTab[S1[i + 1]];
	}
	for (size_t i = 0; i < l2; i++) {
		b2[i] = CharOrdTab[S2[i + 1]];
	}

	auto result = CmpLxStr(b1, l1, b2, l2);

	delete[] b1;
	delete[] b2;
	return result;
}

WORD CompLexStrings(const std::string& S1, const std::string& S2)
{
	WORD l1 = S1.length();
	char* b1 = new char[l1];
	WORD l2 = S2.length();
	char* b2 = new char[l2];

	for (size_t i = 0; i < l1; i++) {
		b1[i] = CharOrdTab[S1[i]];
	}
	for (size_t i = 0; i < l2; i++) {
		b2[i] = CharOrdTab[S2[i]];
	}

	auto result = CmpLxStr(b1, l1, b2, l2);

	delete[] b1;
	delete[] b2;
	return result;
}


void XScan::GetRec()
{
	XString xx;
	CFile = FD;
#ifdef FandSQL
	if (Kind == 4) {
		repeat EOF = !SQLStreamPtr(Strm)->GetRec
			until EOF || hasSQLFilter || RunBool(Bool);
		inc(IRec); return;
	}
#endif
label1:
	eof = IRec >= NRecs;
	if (!eof) {
		IRec++;
		switch (Kind) {
		case 0: { RecNr = IRec; goto label2; break; }
		case 1:
		case 2: {
			RecNr = X->GetN();
			NOnPg--;
			if (NOnPg > 0) X = X->Next(oLeaf, P->IsLeaf);
			else if ((Kind == 2) && (NOfKI == 0)) NextIntvl();
			else if (P->GreaterPage > 0) SeekOnPage(P->GreaterPage, 1);
		label2:
			ReadRec(CFile, RecNr, CRecPtr);
			if (DeletedFlag()) goto label1;
		label3:
			if (!RunBool(Bool)) goto label1;
			break;
		}
#ifdef FandSQL
		case 3: {
			NOnPg--;
			xx.S = P->StrI(P->NItems - NOnPg);
			if ((NOnPg == 0) && (P->GreaterPage > 0)) SeekOnPage(P->GreaterPage, 1);
			if (!Strm1->SelectXRec(Key, @xx, _equ, withT)) goto label1;
			goto label3;
			break;
		}
#endif
		case 5:
		{
			Move(Strm, CRecPtr, CFile->RecLen + 1);
			break;
		}
		}
	}
}

void XScan::SeekOnKI(longint I)
{
	NOfKI = KI->N - I; Key->NrToPath(KI->XNrBeg + I);
	/* !!! with XPath[XPathN] do!!! */
	SeekOnPage(XPath[XPathN].Page, XPath[XPathN].I);
}

void XScan::SeekOnPage(longint Page, WORD I)
{
	Key->XF()->RdPage(P, Page);
	NOnPg = P->NItems - I + 1;
	if (Kind == 2)
	{
		if (NOnPg > NOfKI) NOnPg = NOfKI;
		NOfKI -= NOnPg;
	}
	X = P->XI(I, P->IsLeaf);
}

void XScan::NextIntvl()
{
	XString xx;
	bool b = false;
	longint n = 0, nBeg = 0;

	if (OwnerLV != nullptr) {
		XWKey* k = (XWKey*)OwnerLV->RecPtr; // bude toto fungovat?
		while (iOKey < k->NRecs()) {
			iOKey++;
			CFile = OwnerLV->FD;
			xx.S = k->NrToStr(iOKey);
			CFile = FD;
			Key->FindNr(xx, nBeg);
			AddFFs(Key, xx.S);
			b = Key->FindNr(xx, n);
			n = n - nBeg + b;
			if (n > 0) {
				NOfKI = n;
				Key->NrToPath(nBeg); /* !!! with XPath[XPathN] do!!! */
				SeekOnPage(XPath[XPathN].Page, XPath[XPathN].I);
				return;
			};
		}
		NRecs = IRec; /*EOF*/
	}
	else {
		do { KI = (KeyInD*)KI->Chain; } while (!((KI == nullptr) || (KI->N > 0)));
		if (KI != nullptr) SeekOnKI(0);
	}
}


pstring FieldDMask(FieldDescr* F)
{
	BYTE startIndex = F->Name[0] + 1;
	BYTE newLen = F->Name[startIndex];

	pstring result;
	result[0] = newLen;
	memcpy(&result[1], &F[startIndex], newLen);

	if (result.empty()) return "DD.MM.YY";
	return result;
}

void* GetRecSpace()
{
	//return GetZStore(CFile->RecLen + 2);
	return new BYTE * [CFile->RecLen + 2];
}

void* GetRecSpace2()
{
	return GetZStore2(CFile->RecLen + 2);
}

WORD CFileRecSize()
{
	return 0;
}

void SetTWorkFlag()
{
	BYTE* p = (BYTE*)CRecPtr;
	p[CFile->RecLen] = 1;
}

bool HasTWorkFlag()
{
	BYTE* p = (BYTE*)CRecPtr;
	return p[CFile->RecLen] == 1;
}

void SetUpdFlag()
{
	BYTE* p = (BYTE*)CRecPtr;
	p[CFile->RecLen + 1] = 1;
}

void ClearUpdFlag()
{
	BYTE* p = (BYTE*)CRecPtr;
	p[CFile->RecLen + 1] = 0;
}

bool HasUpdFlag()
{
	BYTE* p = (BYTE*)CRecPtr;
	return p[CFile->RecLen + 1] == 1;
}

void* LocVarAd(LocVar* LV)
{
	return nullptr;
}

void rol(BYTE& input, size_t count)
{
	for (size_t i = 0; i < count; i++) {
		input = input << 1 | input >> 7;
	}
}

void XDecode(LongStr* origS)
{
	BYTE RMask = 0;
	WORD* AX = new WORD();
	BYTE* AL = (BYTE*)AX;
	BYTE* AH = AL + 1;

	WORD* BX = new WORD();
	BYTE* BL = (BYTE*)BX;
	BYTE* BH = BL + 1;

	WORD* CX = new WORD();
	BYTE* CL = (BYTE*)CX;
	BYTE* CH = CL + 1;

	WORD* DX = new WORD();
	BYTE* DL = (BYTE*)DX;
	BYTE* DH = DL + 1;

	if (origS->LL == 0) return;
	BYTE* S = new BYTE[origS->LL + 2];
	WORD* len = (WORD*)&S[0];
	*len = origS->LL;
	memcpy(&S[2], &origS->A[0], origS->LL);

	WORD offset = 0;

	BYTE* DS = S; // ukazuje na delku pred retezcem
	BYTE* ES = &S[2]; // ukazuje na zacatek retezce
	WORD SI = 0;
	*BX = SI;
	*AX = *(WORD*)&DS[SI];
	if (*AX == 0) return;
	SI = 2;
	WORD DI = 0;
	*BX += *AX;
	*AX = *(WORD*)&S[*BX];
	*AX = *AX ^ 0xCCCC;
	SI += *AX;
	(*BX)--;
	*CL = S[*BX];
	*CL = *CL & 3;
	*AL = 0x9C;
	rol(*AL, *CL);
	RMask = *AL;
	*CH = 0;

label1:
	*AL = DS[SI]; SI++; // lodsb
	*DH = 0xFF;
	*DL = *AL;

label2:
	if (SI >= *BX) goto label4;
	if ((*DH & 1) == 0) goto label1;
	if ((*DL & 1) != 0) goto label3;
	*AL = DS[SI]; SI++; // lodsb
	rol(RMask, 1);
	*AL = *AL ^ RMask;
	ES[DI] = *AL; DI++; // stosb
	*DX = *DX >> 1;
	goto label2;

label3:
	*AL = DS[SI]; SI++; // lodsb
	*CL = *AL;
	*AX = *(WORD*)&DS[SI]; SI++; SI++; // lodsw
	offset = *AX;
	for (size_t i = 0; i < *CX; i++) { // rep
		ES[DI] = DS[offset]; offset++; DI++; // movsb
	}
	*DX = *DX >> 1;
	goto label2;

label4:
	origS->LL = DI;
	memcpy(origS->A, &S[2], DI);

	/**AX = DI;
	DI = *len;
	*AX -= DI;
	*AX -= 2;
	ES[DI] = *AL; DI++;
	ES[DI] = *AH; DI++;*/

	delete AX; delete BX; delete CX; delete DX;
	delete[] S;
}

void CodingLongStr(LongStr* S)
{
	if (CFile->TF->LicenseNr == 0) Code(S->A, S->LL);
	else XDecode(S); // musí mít o 2B víc - sahá si tm XDecode!!!
}

void DirMinusBackslash(pstring& D)
{
	if ((D.length() > 3) && (D[D.length() - 1] == '\\')) D[0]--;
}

longint StoreInTWork(LongStr* S)
{
	return TWork.Store(S);
}

LongStrPtr ReadDelInTWork(longint Pos)
{
	auto result = TWork.Read(1, Pos);
	TWork.Delete(Pos);
	return result;
}

void ForAllFDs(void(*procedure)())
{
	RdbDPtr R; FileDPtr cf;
	cf = CFile; R = CRdb;
	while (R != nullptr) {
		CFile = R->FD;
		while (CFile != nullptr) { procedure(); CFile = (FileD*)CFile->Chain; };
		R = R->ChainBack;
	}
	CFile = cf;
}

bool IsActiveRdb(FileDPtr FD)
{
	RdbDPtr R;
	R = CRdb; while (R != nullptr) {
		if (FD == R->FD) return true;
		R = R->ChainBack;
	}
	return false;
}

void ResetCompilePars()
{
	RdFldNameFrml = RdFldNameFrmlF;
	RdFunction = nullptr;
	ChainSumEl = nullptr;
	FileVarsAllowed = true;
	FDLocVarAllowed = false;
	IdxLocVarAllowed = false;
	PrevCompInp = nullptr;
}

std::string TranslateOrd(std::string text)
{
	std::string trans;
	for (size_t i = 0; i < text.length(); i++) {
		char c = CharOrdTab[text[i]];
#ifndef FandAng
		if (c == 0x49 && trans.length() > 0) { // znak 'H'
			if (trans[trans.length() - 1] == 0x43) { // posledni znak ve vystupnim retezci je 'C' ?
				trans[trans.length() - 1] = 0x4A; // na vstupu bude 'J' jako 'CH'
				continue;
			}
		}
#endif
		trans += c;
	}
	return trans;
}

FieldDescr::FieldDescr()
{
}

FieldDescr::FieldDescr(BYTE* inputStr)
{
	size_t index = 0;
	Chain = reinterpret_cast<FieldDescr*>(*(unsigned int*)&inputStr[index]); index += 4;
	Typ = *(char*)&inputStr[index]; index++;
	FrmlTyp = *(char*)&inputStr[index]; index++;
	L = *(char*)&inputStr[index]; index++;
	M = *(char*)&inputStr[index]; index++;
	NBytes = *(char*)&inputStr[index]; index++;
	Flg = *(char*)&inputStr[index]; index++;

	unsigned int DisplOrFrml = *(unsigned int*)&inputStr[index]; index += 4;
	if (DisplOrFrml > MaxTxtCols) {
		// jedna se o ukazatel
		Frml = reinterpret_cast<FrmlElem*>(DisplOrFrml);
	}
	else {
		// jedna se o delku
		Displ = DisplOrFrml;
	}
	Name[0] = inputStr[index]; index++;
	memcpy(&Name[1], &inputStr[index], Name[0]); index += Name[0];
}

FieldDescr::FieldDescr(const FieldDescr& orig)
{
	if (orig.Chain != nullptr) Chain = new FieldDescr(*(FieldDescr*)orig.Chain);
	Typ = orig.Typ;
	FrmlTyp = orig.FrmlTyp;
	L = orig.L; M = orig.M; NBytes = orig.NBytes; Flg = orig.Flg;
	Displ = orig.Displ;
	Frml = CopyFrmlElem(orig.Frml);
	Name = orig.Name;
}

KeyFldD::KeyFldD(const KeyFldD& orig, bool copyFlds)
{
	if (orig.Chain != nullptr) Chain = new KeyFldD(*(KeyFldD*)orig.Chain);
	// v objektu FileD jsou asi ukazatele FldD a Keys->KFlds->FldD stejne
	if (copyFlds && orig.FldD != nullptr) FldD = new FieldDescr(*orig.FldD);
	CompLex = orig.CompLex;
	Descend = orig.Descend;
}

KeyFldD::KeyFldD(BYTE* inputStr)
{
	size_t index = 0;
	Chain = reinterpret_cast<KeyFldD*>(*(unsigned int*)&inputStr[index]); index += 4;
	FldD = reinterpret_cast<FieldDescr*>(*(unsigned int*)&inputStr[index]); index += 4;
	CompLex = *(bool*)&inputStr[index]; index++;
	Descend = *(bool*)&inputStr[index]; index++;
}

AddD::AddD(const AddD& orig)
{
	if (orig.Chain != nullptr) Chain = new AddD(*orig.Chain);
	if (orig.Chain != nullptr) Field = new FieldDescr(*orig.Field);
	if (orig.Chain != nullptr) File2 = new FileD(*orig.File2);
	if (orig.Chain != nullptr) LD = new LinkD(*orig.LD);
	Create = orig.Create;
	if (orig.Chain != nullptr) Frml = CopyFrmlElem(orig.Frml);
	Assign = orig.Assign;
	if (orig.Chain != nullptr) Bool = CopyFrmlElem(orig.Bool);
	if (orig.Chain != nullptr) Chk = new ChkD(*orig.Chk);
}

ChkD::ChkD(const ChkD& orig)
{
	if (orig.Bool != nullptr) Bool = CopyFrmlElem(orig.Bool);
	HelpName = orig.HelpName;
	if (orig.TxtZ != nullptr) TxtZ = CopyFrmlElem(orig.TxtZ);
	Warning = orig.Warning;
}
