#include "access.h"


#include "../pascal/random.h"
#include "compile.h"
#include "legacy.h"
#include "oaccess.h"
#include "obaseww.h"
#include "olongstr.h"
#include "runfrml.h"
#include "sort.h"

// Random z TP7 pro Seed = 739328 + 205
BYTE randomice739k[512] = { 127, 73, 105, 45, 152, 197, 176, 159, 158, 83, 66, 240, 82, 71, 130, 28,
	149, 163, 70, 61, 81, 145, 16, 32, 122, 85, 119, 176, 96, 218, 146, 143, 209, 72, 138, 93,
	196, 70, 174, 135, 135, 227, 162, 165, 181, 75, 135, 168, 108, 168, 51, 119, 110, 242, 222,
	181, 79, 142, 222, 206, 193, 220, 65, 131, 156, 61, 116, 142, 74, 29, 73, 48, 150, 7, 245,
	160, 39, 225, 236, 185, 17, 190, 172, 111, 66, 240, 219, 209, 203, 84, 83, 171, 50, 236, 241,
	18, 85, 77, 79, 95, 158, 115, 181, 120, 121, 0, 242, 85, 67, 1, 229, 49, 74, 167, 217, 205,
	117, 175, 28, 184, 177, 71, 79, 131, 67, 189, 121, 71, 146, 86, 195, 2, 27, 231, 104, 142,
	129, 72, 85, 76, 14, 147, 105, 196, 4, 218, 104, 205, 147, 198, 247, 7, 98, 55, 79, 166,
	232, 242, 112, 3, 19, 229, 0, 60, 252, 141, 28, 246, 165, 139, 218, 30, 236, 122, 52, 252,
	75, 128, 134, 64, 10, 67, 11, 168, 231, 45, 211, 123, 123, 38, 54, 247, 183, 64, 181, 102,
	101, 232, 207, 138, 127, 170, 71, 122, 169, 134, 196, 55, 84, 117, 233, 135, 45, 165, 55,
	225, 236, 98, 98, 121, 182, 228, 234, 172, 131, 93, 27, 112, 89, 245, 197, 118, 75, 185,
	98, 37, 115, 127, 95, 165, 117, 77, 200, 155, 52, 223, 160, 72, 192, 70, 132, 45, 186, 176,
	116, 125, 159, 231, 233, 218, 194, 33, 132, 58, 235, 3, 245, 242, 219, 33, 10, 77, 41, 75,
	223, 9, 58, 92, 175, 197, 87, 123, 197, 57, 13, 252, 124, 153, 90, 64, 97, 187, 109, 76,
	213, 169, 230, 1, 208, 208, 217, 27, 145, 7, 14, 107, 110, 239, 62, 244, 16, 153, 73, 215,
	183, 80, 148, 51, 111, 5, 34, 122, 65, 188, 12, 207, 201, 237, 102, 101, 198, 188, 198, 15,
	133, 128, 230, 89, 58, 4, 36, 251, 179, 85, 208, 102, 238, 60, 158, 177, 127, 149, 141, 93,
	209, 29, 51, 115, 177, 130, 55, 21, 174, 200, 96, 147, 56, 59, 154, 119, 137, 139, 178, 52,
	204, 222, 206, 101, 5, 218, 217, 200, 162, 247, 84, 101, 219, 172, 91, 118, 35, 57, 202, 40,
	99, 29, 202, 54, 193, 140, 52, 219, 41, 100, 148, 213, 214, 114, 198, 71, 151, 28, 91, 189,
	145, 170, 84, 192, 172, 209, 226, 137, 250, 52, 56, 235, 194, 243, 138, 116, 253, 157, 225,
	79, 104, 253, 90, 141, 139, 156, 223, 179, 70, 4, 156, 201, 202, 1, 140, 83, 23, 135, 11,
	212, 85, 238, 57, 223, 177, 155, 138, 192, 139, 108, 245, 80, 28, 91, 117, 123, 236, 65,
	230, 132, 182, 172, 119, 25, 142, 35, 34, 181, 113, 91, 247, 241, 150, 50, 41, 184, 22, 120,
	25, 131, 205, 200, 231, 15, 155, 20, 63, 50, 62, 149, 74, 199, 145, 66, 45, 206, 226, 12 };
// pro Seed = 4173
//BYTE randomice7s4173[512] = { 241, 60, 19, 186, 47, 192, 151, 86, 91, 83, 41, 164, 119, 47, 193, 49, 92, 196, 135, 74, 6, 66, 48, 84, 130, 11, 12, 129, 170, 76, 158, 51, 39, 27, 214, 134, 135, 192, 76, 241, 90, 43, 21, 225, 138, 163, 18, 179, 115, 65, 214, 156, 122, 189, 122, 51, 23, 150, 247, 199, 143, 170, 246, 179, 228, 196, 22, 80, 164, 175, 28, 62, 165, 207, 154, 38, 233, 103, 242, 45, 18, 238, 204, 176, 57, 55, 88, 14, 33, 49, 1, 147, 236, 156, 105, 201, 241, 241, 200, 196, 75, 72, 8, 37, 58, 22, 64, 243, 132, 0, 104, 143, 78, 30, 97, 62, 58, 97, 172, 59, 176, 12, 110, 49, 161, 100, 154, 144, 53, 14, 103, 39, 38, 183, 88, 233, 186, 5, 145, 227, 171, 234, 57, 158, 101, 213, 17, 145, 89, 102, 161, 99, 121, 135, 142, 4, 241, 53, 245, 246, 189, 56, 239, 75, 192, 159, 149, 30, 106, 218, 29, 158, 23, 148, 240, 244, 178, 206, 218, 138, 85, 223, 162, 134, 211, 59, 177, 126, 134, 57, 165, 191, 186, 67, 227, 157, 141, 24, 142, 171, 53, 8, 117, 227, 224, 97, 222, 254, 187, 119, 68, 156, 208, 223, 97, 245, 186, 241, 46, 43, 107, 141, 158, 131, 128, 184, 75, 26, 225, 169, 87, 206, 168, 47, 50, 134, 133, 168, 22, 254, 37, 239, 83, 208, 83, 234, 208, 88, 206, 169, 90, 165, 17, 66, 147, 173, 136, 210, 173, 83, 245, 82, 68, 24, 245, 43, 238, 109, 232, 181, 111, 10, 186, 15, 148, 192, 72, 246, 42, 168, 82, 148, 149, 159, 75, 93, 0, 124, 111, 128, 27, 111, 231, 133, 240, 106, 242, 8, 73, 146, 80, 197, 133, 9, 107, 94, 25, 158, 254, 113, 233, 52, 47, 26, 64, 220, 83, 216, 31, 3, 246, 103, 35, 39, 184, 39, 59, 0, 25, 5, 227, 7, 102, 14, 183, 185, 74, 26, 116, 44, 170, 58, 208, 158, 238, 212, 190, 55, 195, 159, 244, 42, 2, 221, 221, 201, 77, 147, 71, 100, 134, 152, 103, 234, 58, 180, 42, 173, 139, 53, 189, 194, 98, 165, 225, 220, 193, 147, 121, 155, 157, 148, 69, 172, 153, 239, 228, 55, 110, 151, 121, 119, 251, 106, 126, 118, 60, 212, 194, 74, 141, 85, 145, 78, 33, 123, 103, 58, 124, 87, 222, 10, 70, 140, 154, 25, 234, 92, 86, 231, 201, 118, 124, 110, 224, 87, 214, 244, 67, 126, 47, 199, 203, 211, 15, 77, 198, 247, 236, 84, 149, 88, 95, 152, 131, 79, 65, 210, 71, 151, 210, 226, 241, 228, 64, 208, 62, 111, 36, 196, 138, 178, 220, 52, 234, 189, 219, 212, 21, 202, 67, 30, 41, 1, 112, 110, 131, 155, 75, 19, 35, 200, 128, 199, 90, 13, 246, 123, 192, 247, 30, 88, 175, 106, 1, 113, 176, 213, 31, 140, 181, 24, 126, 221, 247, 239, 137, 201, 213, 33, 142, 226 };
//BYTE randomice6s4173[512] = { 195, 52, 146, 75, 228, 104, 69, 96, 14, 185, 89, 194, 222, 108, 146, 81, 4, 24, 139, 33, 138, 31, 191, 216, 2, 89, 106, 186, 174, 15, 64, 132, 84, 191, 1, 219, 66, 98, 80, 227, 166, 250, 105, 51, 109, 194, 135, 86, 11, 65, 40, 130, 23, 20, 118, 43, 136, 49, 140, 168, 249, 254, 83, 15, 68, 126, 45, 243, 87, 77, 191, 195, 46, 117, 115, 201, 43, 14, 150, 53, 74, 79, 41, 155, 252, 146, 195, 209, 132, 81, 37, 40, 59, 55, 66, 61, 16, 189, 7, 26, 33, 251, 12, 68, 88, 148, 244, 227, 60, 239, 54, 55, 145, 42, 126, 251, 108, 105, 7, 124, 189, 128, 120, 95, 136, 15, 44, 131, 85, 130, 253, 103, 131, 179, 245, 0, 129, 250, 114, 238, 45, 250, 169, 122, 209, 111, 155, 201, 121, 215, 222, 43, 157, 187, 201, 134, 96, 212, 241, 40, 221, 132, 10, 137, 66, 46, 125, 224, 164, 125, 113, 110, 234, 180, 242, 240, 35, 113, 123, 186, 27, 146, 49, 54, 53, 46, 98, 234, 160, 201, 56, 71, 142, 47, 163, 196, 5, 152, 234, 40, 101, 33, 127, 247, 200, 153, 217, 94, 190, 53, 150, 27, 44, 191, 36, 72, 56, 61, 142, 236, 143, 35, 105, 208, 116, 162, 200, 234, 91, 152, 199, 106, 9, 38, 40, 55, 122, 24, 232, 92, 4, 116, 238, 203, 107, 252, 38, 92, 254, 124, 161, 2, 211, 9, 179, 134, 193, 177, 252, 105, 181, 64, 225, 143, 121, 252, 1, 234, 30, 148, 112, 87, 118, 155, 8, 63, 124, 91, 229, 200, 131, 185, 237, 177, 123, 152, 6, 2, 202, 226, 68, 60, 104, 28, 78, 211, 68, 72, 159, 230, 45, 100, 132, 147, 187, 206, 233, 125, 231, 92, 91, 50, 103, 240, 204, 154, 245, 227, 24, 171, 8, 115, 42, 244, 172, 41, 96, 199, 153, 224, 159, 15, 142, 210, 251, 37, 158, 225, 32, 35, 23, 248, 186, 146, 237, 88, 157, 107, 23, 238, 100, 196, 21, 84, 188, 187, 161, 217, 180, 80, 81, 213, 159, 98, 133, 33, 113, 243, 15, 94, 186, 95, 89, 159, 130, 57, 253, 82, 189, 234, 23, 179, 36, 97, 180, 38, 140, 71, 70, 21, 41, 165, 229, 100, 66, 236, 188, 128, 70, 190, 31, 131, 77, 246, 225, 45, 115, 198, 36, 122, 149, 240, 145, 30, 93, 103, 202, 64, 176, 232, 130, 51, 223, 83, 113, 4, 48, 209, 67, 23, 155, 23, 5, 65, 240, 64, 201, 248, 146, 52, 143, 161, 139, 106, 136, 17, 209, 146, 94, 177, 194, 126, 29, 4, 63, 58, 184, 91, 18, 72, 139, 3, 91, 96, 28, 61, 65, 41, 10, 199, 37, 200, 19, 200, 13, 89, 94, 96, 14, 113, 158, 55, 224, 128, 194, 199, 243, 141, 32, 72, 151, 155, 106, 80, 47, 213, 225, 209, 93, 47, 131, 213, 187, 39, 41, 39, 140, 208, 220, 212, 94, 105 };;

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


integer CompLongStr(LongStrPtr S1, LongStrPtr S2)
{
	return 0;
}

integer CompLongShortStr(LongStrPtr S1, pstring S2)
{
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
	CloseClearHCFile(); GoExit();
}

void TFile::Err(WORD n, bool ex)
{
	if (IsWork) {
		SetMsgPar(FandWorkTName); WrLLF10Msg(n); if (ex) GoExit();
	}
	else { CFileMsg(n, 'T'); if (ex) CloseGoExit(); }
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
	if (FLen == 0) return 0;
	BYTE* source = (BYTE*)FixNo;
	double r = 0;
	// pricteme 1. byte
	r += source[0];
	// dalsi byte se postupne nasobi 256 na i-tou
	for (size_t i = 1; i < FLen; i++) 
	{
		r += source[i] * pow(256, i);
	}

	return r;

	//double r = 0;
	//BYTE* rr = (BYTE*)&r;
	//BYTE ff[FixS]{ '\0' };
	//integer i = 0;

	////FillChar(rr, DblS, 0);
	//Move(FixNo, &ff[0], FLen);
	//bool neg = (ff[1] & 0x80) != 0;
	//if (neg) {
	//	if (ff[1] == 0x80) {
	//		for (i = 2; i < FLen; i++) if (ff[i] != 0x00) goto label1;   /*NULL value*/
	//		return 0.0;
	//	}
	//label1:
	//	for (i = 1; i < FLen; i++) ff[i] = !(ff[i]);
	//	ff[FLen]++;
	//	i = FLen;
	//	while (ff[i] == 0) { i--; if (i > 0) ff[i]++; }
	//}
	//integer first = 1;
	//while (ff[first] == 0) first++;
	//if (first > FLen) { return 0; }
	//integer lef = 0;
	//BYTE b = ff[first];
	//while ((b & 0x80) == 0) { b = b << 1; lef++; }
	//ff[first] = ff[first] && (0x7F >> lef);
	//integer exp = ((FLen - first) << 3) - lef + 1030;
	//if (lef == 7) first++;
	//lef = (lef + 5) & 0x07;
	//integer rig = 8 - lef;
	//i = DblS - 1;
	//if ((rig <= 4) && (first <= FLen)) { rr[i] = ff[first] >> rig; i--; }
	//while ((i > 0) && (first < FLen))
	//{
	//	rr[i] = (ff[first] << lef) + (ff[first + 1] >> rig);
	//	i--;
	//	first++;
	//}
	//if ((first == FLen) && (i > 0)) rr[i] = ff[first] << lef;
	//rr[DblS - 1] = (rr[DblS - 1] & 0x0F) + ((exp & 0x0F) << 4);
	//rr[DblS] = exp >> 4;
	//if (neg) rr[DblS] = rr[DblS] | 0x80;
	//return r;
}

void FixFromReal(double r, void* FixNo, WORD& flen)
{
	BYTE* rr = (BYTE*)&r;
	BYTE* ff = (BYTE*)FixNo;

	FillChar(ff, flen, 0);
	if (r > 0) r = r + 0.5;
	else r = r - 0.5;
	bool neg = bool(rr[DblS] & 0x80);
	integer exp = (rr[DblS - 1] >> 4) + (WORD(rr[DblS] & 0x7F) << 4);
	if (exp < 2047)
	{
		rr[DblS] = 0; rr[DblS - 1] = rr[DblS - 1] & 0x0F;
		if (exp > 0) { rr[DblS - 1] = rr[DblS - 1] | 0x10; }
		else { exp++; }
		exp -= 1023;
		if (exp > (flen << 3) - 1) /*overflow*/ return;
		integer lef = (exp + 4) & 0x0007;
		integer rig = 8 - lef;
		if ((exp & 0x0007) > 3) exp += 4;
		integer first = 7 - (exp >> 3);
		integer i = flen;
		while ((first < DblS) and (i > 0))
		{
			ff[i] = (rr[first] >> rig) + (rr[first + 1] << lef);
			i--; first++;
		}
		if (i > 0) ff[i] = rr[first] >> rig;
		if (neg)
		{
			for (i = 1; i < flen; i++) ff[i] = !ff[i]; ff[flen]++;
			i = flen;
			while (ff[i] == 0) {
				i--;
				if (i > 0) ff[i]++;
			}
		}
	}
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
	longint FreeRoot = 0, MaxPage = 0;   /*eldest version=>array Pw[1..40] of char;*/
	double TimeStmp = 0.0;
	bool HasCoproc = false;
	char Rsrvd2[25]{ '\0' };
	char Version[4]{ '\0' };
	BYTE LicText[105]{ '\0' };
	BYTE Sum = 0;
	char X1[295]{ '\0' };
	WORD LicNr = 0;
	char X2[11]{ '\0' };
	char PwNew[40]{ '\0' };
	BYTE Time = 0;

	void Load(BYTE* input512);
};

// nahraje prvnich 24 Bytu ze souboru do struktury
// (nav�c i hodnotu HasCoproc, aby byla pripravena pro porovnani)
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
	memcpy(&X1, &input512[index], 195); index += 295;
	memcpy(&LicNr, &input512[index], 2); index += 2;
	memcpy(&X2, &input512[index], 11); index += 11;
	memcpy(&PwNew, &input512[index], 40); index += 40;
	memcpy(&Time, &input512[index], 1); index++;
	// index by te� m�l m�t 512
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
	/* !!! with CFile^ do!!! */
	ClearCacheH(CFile->Handle);
	if (CFile->Typ == 'X') ClearCacheH(CFile->XF->Handle);
	if (CFile->TF != nullptr) ClearCacheH(CFile->TF->Handle);
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
	struct x6 { longint NRs = 0; WORD RLen = 0; } X6;
	struct x8 { WORD NRs = 0, RLen = 0; } X8;
	struct xD {
		BYTE Ver = 0; BYTE Date[3] = { 0,0,0 };
		longint NRecs = 0;
		WORD HdLen = 0; WORD RecLen = 0;
	} XD;
	auto result = 0xffff;
	/* !!! with CFile^ do!!! */
	switch (CFile->Typ) {
	case '8': {
		RdWrCache(true, CFile->Handle, CFile->NotCached(), 0, 4, &X8);
		CFile->NRecs = X8.NRs;
		if (CFile->RecLen != X8.RLen) { return X8.RLen; }
		break;
	}
	case 'D': {
		RdWrCache(true, CFile->Handle, CFile->NotCached(), 0, 12, &XD);
		CFile->NRecs = XD.NRecs;
		if ((CFile->RecLen != XD.RecLen)) { return XD.RecLen; }
		CFile->FrstDispl = XD.HdLen;
		break;
	}
	default: {
		RdWrCache(true, CFile->Handle, CFile->NotCached(), 0, 6, &X6);
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
		if (F->Flg && f_Stored != 0) {
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
		RdWrCache(false, CFile->Handle, CFile->NotCached(), 0, CFile->FrstDispl, (void*)&P);
		RdWrCache(false, CFile->Handle, CFile->NotCached(),
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
		switch (CFile->Typ)
		{
		case '8': {
			Pfx8.RLen = CFile->RecLen;
			Pfx8.NRs = CFile->NRecs;
			RdWrCache(false, CFile->Handle, CFile->NotCached(), 0, 4, (void*)&Pfx8);
			break;
		}
		case 'D': {
			WrDBaseHd(); break;
		}
		default: {
			Pfx6.RLen = CFile->RecLen;
			if (CFile->Typ == 'X') Pfx6.NRs = -CFile->NRecs;
			else Pfx6.NRs = CFile->NRecs;
			RdWrCache(false, CFile->Handle, CFile->NotCached(), 0, 6, (void*)&Pfx6);
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

void CExtToX()
{
	CExt[2] = 'X'; CPath = CDir + CName + CExt;
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
		SetMsgPar(CPath); RunError(n);
	}
}

void CExtToT()
{
	if (SEquUpcase(CExt, ".RDB"))
		CExt = ".TTT";
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

void ReadRec(longint N)
{
	/* with CFile^ do */
	RdWrCache(true, CFile->Handle, CFile->NotCached(),
		(N - 1) * CFile->RecLen + CFile->FrstDispl, CFile->RecLen, CRecPtr);
}

void WriteRec(longint N)
{
	RdWrCache(false, CFile->Handle, CFile->NotCached(),
		(N - 1) * CFile->RecLen + CFile->FrstDispl, CFile->RecLen, CRecPtr);
	CFile->WasWrRec = true;
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

// v CRecPtr se posune o F->Displ a vy�te integer
longint _T(FieldDescr* F)
{
	void* p = CRecPtr;
	longint n = 0;
	integer err = 0;
	char* source = (char*)p + F->Displ;

	if (CFile->Typ == 'D')
	{
		// tv���me se, �e CRecPtr je pstring ...
		// TODO: toto je asi blb�, nutno opravit p�ed 1. pou�it�m
		pstring* s = (pstring*)CRecPtr;
		auto result = std::stoi(LeadChar(' ', *s));
		return result;
	}
	else
	{
		if (IsNullValue(source, 4)) return 0;
		longint* lip = (longint*)source;
		return *lip;
	}
}

void T_(FieldDPtr F, longint Pos)
{
	pstring s;
	void* p = CRecPtr;
	char* source = (char*)p + F->Displ;
	longint* LP = (longint*)source;
	if ((F->Typ == 'T') && (F->Flg && f_Stored != 0)) {
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
	longint n; void* cr;
	cr = CRecPtr; CRecPtr = CompRec; n = _T(F); CRecPtr = Rec;
	if (n != _T(F)) DelTFld(F); CRecPtr = cr;
}

void DelAllDifTFlds(void* Rec, void* CompRec)
{
	FieldDescr* F = CFile->FldD;
	while (F != nullptr)
	{
		if (F->Typ == 'T' && F->Flg && f_Stored != 0) DelDifTFld(Rec, CompRec, F);
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
	for (WORD i = s.length() + 1; i < l; i++) s[i] = 0xff;
	s[0] = char(l);
}

void CompKIFrml(KeyDPtr K, KeyInD* KI, bool AddFF)
{
	XString x; bool b; integer i;
	while (KI != nullptr) {
		b = x.PackFrml(KI->FL1, K->KFlds);
		KI->X1 = &x.S;
		if (KI->FL2 != nullptr) x.PackFrml(KI->FL2, K->KFlds);
		if (AddFF) AddFFs(K, x.S);
		KI->X2 = &x.S;
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
	CFile->IRec++; CFile->Eof = true;
}

void CreateRec(longint N)
{
	IncNRecs(1);
	void* cr = CRecPtr;
	CRecPtr = GetRecSpace();
	for (longint i = CFile->NRecs - 1; i > N; i--) {
		ReadRec(i);
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
		ReadRec(i + 1); WriteRec(i);
	}
	DecNRecs(1);
}

void LongS_(FieldDPtr F, LongStr* S)
{
	longint Pos; LockMode md;

	if (F->Flg && f_Stored != 0) {
		if (S->LL == 0) T_(F, 0);
		else {
			if (F->Flg && f_Encryp != 0) Code(S->A, S->LL);
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
			if (F->Flg && f_Encryp != 0) Code(S->A, S->LL); T_(F, Pos);
		}
	}
}

void S_(FieldDPtr F, pstring S)
{
	void* p = nullptr;
	WORD* O = (WORD*)p; double* RP = (double*)p;
	integer i, L, M; longint Pos; LongStrPtr ss;
	const BYTE LeftJust = 1;

	if (F->Flg && f_Stored != 0)
	{
		p = CRecPtr; O += F->Displ; L = F->L; M = F->M;
		switch (F->Typ) {
		case 'A': {
			while (S.length() < L)
				if (M == LeftJust) S = S + " ";
				else
				{
					pstring oldS = S;
					S = " ";
					S += oldS;
				}
			i = 1;
			if ((S.length() > L) && (M != LeftJust)) i = S.length() + 1 - L;
			Move(&S[i], p, L);
			if (F->Flg && f_Encryp != 0) Code(p, L);
			break;
		}
		case 'N': {
			while (S.length() < L)
				if (M == LeftJust) S = S + "0";
				else
				{
					pstring oldS = S;
					S = " ";
					S += oldS;
				}
			i = 1;
			if ((S.length() > L) && (M != LeftJust)) i = S.length() + 1 - L;
			//Pack(&S[i], p, L);
			break;
		}
		case 'T': {
			ss = CopyToLongStr(S);
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

bool LinkLastRec(FileDPtr FD, longint& N, bool WithT)
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
		else ReadRec(N);
	}
	OldLMode(md);
	return result;
}

void AsgnParFldFrml(FileD* FD, FieldDPtr F, FrmlPtr Z, bool Ad)
{
	void* p; longint N; LockMode md; bool b;
	FileDPtr cf = CFile; void* cr = CRecPtr; CFile = FD;
#ifdef FandSQL
	if (CFile->IsSQLFile) {
		CRecPtr = GetRecSpace; ZeroAllFlds; AssgnFrml(F, Z, true, Ad);
		Strm1->UpdateXFld(nullptr, nullptr, F); ClearRecSpace(CRecPtr)
	}
	else
#endif
	{
		md = NewLMode(WrMode);
		if (!LinkLastRec(CFile, N, true)) { IncNRecs(1); WriteRec(N); }
		AssgnFrml(F, Z, true, Ad); WriteRec(N); OldLMode(md);
	}
	ReleaseStore(CRecPtr); CFile = cf; CRecPtr = cr;
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
		if (Result == _gt) R = N; else L = N + 1;
		N = (L + R) / 2; ReadRec(N); x.PackKF(KF);
		Result = CompStr(x.S, XX.S);
	} while (!((L >= R) || (Result == _equ)));
	if ((N == NN) && (Result == _lt)) NN++;
	else {
		if (Key->Duplic && (Result == _equ))
			while (N > 1) {
				N--; ReadRec(N); x.PackKF(KF);
				if (CompStr(x.S, XX.S) != _equ) {
					N++; ReadRec(N); goto label1;
				};
			}
	label1:  NN = N;
	}
	if ((Result == _equ) || Key->Intervaltest && (Result == _gt))
		bResult = true;
	return bResult;
}

LongStr* _LongS(FieldDPtr F)
{
	void* P = CRecPtr;
	char* source = (char*)P;
	LongStr* S = nullptr; longint Pos = 0; integer err = 0;
	LockMode md; WORD l = 0;
	if ((F->Flg & f_Stored) != 0) {
		l = F->L;
		switch (F->Typ)
		{
		case 'A': 
		case 'N': {
			S = new LongStr(l + 2);
			S->LL = l;
			if (F->Typ == 'A') {
				Move(&source[F->Displ], &S[0], l);
				if (F->Flg && f_Encryp != 0) Code(S->A, l);
				if (IsNullValue(S, l)) { S->LL = 0; ReleaseAfterLongStr(S); }
			}
			else if (IsNullValue(&source[F->Displ], F->NBytes)) {
				S->LL = 0;
				ReleaseAfterLongStr(S);
			}
			else
			{
				// nebudeme volat, z�ejm�n� nen� pot�eba
				// UnPack(P, S->A, l);
			}
			break;
		}
		case 'T': {
			if (HasTWorkFlag()) S = TWork.Read(1, _T(F));
			else {
				md = NewLMode(RdMode);
				S = CFile->TF->Read(1, _T(F));
				OldLMode(md);
			}
			if ((F->Flg & f_Encryp) != 0) Code(S->A, S->LL);
			if (IsNullValue(&S->A, S->LL))
			{
				S->LL = 0;
				ReleaseAfterLongStr(S);
			}
			break; }
		}
		return S;
	}
	return RunLongStr(F->Frml);
}

// z CRecPtr vy�te �et�zec o d�lce F->L z pozice F->Displ
pstring _ShortS(FieldDPtr F)
{
	void* P = CRecPtr;
	pstring S;
	if ((F->Flg & f_Stored) != 0) {
		WORD l = F->L;
		S[0] = l;
		//WORD* POfs = (WORD*)P; /*absolute P;*/
		//*POfs += F->Displ;
		char* source = (char*)P;
		switch (F->Typ) {
		case 'A':
		case 'N': {
			if (F->Typ == 'A') {
				Move(&source[F->Displ], &S[1], l);
				if ((F->Flg & f_Encryp) != 0) Code(&S[1], l);
				if (IsNullValue(&S[2], l)) FillChar(&S[0], l, ' ');
			}
			else if (IsNullValue(&source[F->Displ], F->NBytes)) FillChar(&S[0], l, ' ');
			else
			{
				// nebudeme volat, z�ejm�n� nen� pot�eba
				// UnPack(P, (WORD*)S[0], l);
			}
			break;
		}
		case 'T': {
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
		if (F->Flg && f_Comma != 0) {
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

double _R(FieldDPtr F)
{
	void* P = CRecPtr;
	char* source = (char*)P;
	double result = 0.0;
	double r;

	if ((F->Flg & f_Stored) != 0) {
		if (CFile->Typ == 'D') result = _RforD(F, &source[F->Displ]);
		else switch (F->Typ) {
		case 'F': {
			r = RealFromFix(&source[F->Displ], F->NBytes);
			if ((F->Flg & f_Comma) == 0) result = r / Power10[F->M]; 
			else result = r; 
			break;
		}
		case 'D': {
			if (CFile->Typ == '8') {
				if (*(integer*)&source[F->Displ] == 0) result = 0.0;
				else result = *(integer*)&source[F->Displ] + FirstDate;
			}
			else goto label1; 
			break;
		}
		case 'R': {
		label1:
			if (IsNullValue(&source[F->Displ], F->NBytes)) result = 0;
			else result = Real48ToDouble(&source[F->Displ]);
		}
		}
	}
	else return RunReal(F->Frml);
	return result;
}

// vrac� BOOL ze z�znamu CRecPtr na pozici F->Displ
bool _B(FieldDPtr F)
{
	bool result;
	void* p = CRecPtr;
	//char* source = (char*)p;
	unsigned char* CP = (unsigned char*)p + F->Displ;
	if ((F->Flg & f_Stored) != 0) {
		//*O += F->Displ;
		if (CFile->Typ == 'D') result = *CP == 'Y' || *CP == 'y' || *CP == 'T' || *CP == 't';
		else if ((*CP == '\0') || (*CP == 0xFF)) result = false;
		else result = true;
	}
	else result = RunBool(F->Frml);
	return result;
}

void R_(FieldDPtr F, double R)
{
	void* p = CRecPtr;
	pstring s; WORD m; longint l;
	WORD* O = (WORD*)p;
	integer* IP = (integer*)p;
	if ((F->Flg & f_Stored) != 0) {
		O += F->Displ;
		m = F->M;
		switch (F->Typ) {
		case 'F': {
			if (CFile->Typ == 'D') {
				if (F->Flg && f_Comma != 0) R = R / Power10[m];
				str(F->NBytes, s);
				Move(&s[1], p, F->NBytes);
			}
			else {
				if (F->Flg && f_Comma == 0) R = R * Power10[m];
				WORD tmp = F->NBytes;
				FixFromReal(R, p, tmp);
				F->NBytes = (BYTE)tmp;
			} }
		case 'D': {
			switch (CFile->Typ) {
			case '8': {
				if (trunc(R) == 0) *IP = 0;
				else *IP = trunc(R - FirstDate);
				break;
			}
			case 'D': {
				s = StrDate(R, "YYYYMMDD");
				Move(&s[1], p, 8);
				break;
			}
			default: p = &R; break;
			}
		}
		case 'R': { p = &R; break; }
		}
	}
}

void B_(FieldDPtr F, bool B)
{
	void* p = CRecPtr;
	WORD* O = (WORD*)p;
	bool* BP = (bool*)p;
	char* CP = (char*)p;
	if ((F->Typ == 'B') && (F->Flg && f_Stored != 0)) {
		*O += F->Displ;
		if (CFile->Typ == 'D')
		{
			if (B) *CP = 'T'; else *CP = 'F';
		}
		else *BP = B;
	}
}

bool LinkUpw(LinkDPtr LD, longint& N, bool WithT)
{
	KeyFldDPtr KF;
	FieldDPtr F, F2;
	bool LU; LockMode md;
	pstring s; double r; bool b;
	XString* x = (XString*)&s;

	FileDPtr ToFD = LD->ToFD; FileDPtr CF = CFile; void* CP = CRecPtr;
	KeyDPtr K = LD->ToKey; KeyFldDPtr Arg = LD->Args; x->PackKF(Arg);
	CFile = ToFD; void* RecPtr = GetRecSpace(); CRecPtr = RecPtr;
#ifdef FandSQL
	if (CFile->IsSQLFile) {
		LU = Strm1->SelectXRec(K, @X, _equ, WithT); N = 1; if (LU) goto label2; else goto label1;
	}
#endif
	md = NewLMode(RdMode);
	if (ToFD->Typ == 'X') { TestXFExist(); LU = K->SearchIntvl(*x, false, N); }
	else if (CFile->NRecs = 0) { LU = false; N = 1; }
	else LU = SearchKey(*x, K, N);
	if (LU) ReadRec(N);
	else {
	label1:
		ZeroAllFlds; KF = K->KFlds; while (Arg != nullptr) {
			F = Arg->FldD; F2 = KF->FldD; CFile = CF; CRecPtr = CP;
			if (F2->Flg && f_Stored != 0)
				switch (F->FrmlTyp) {
				case 'S': { s = _ShortS(F); CFile = ToFD; CRecPtr = RecPtr; S_(F2, s); break; }
				case 'R': { r = _R(F); CFile = ToFD; CRecPtr = RecPtr; R_(F2, r); break; }
				case 'B': { b = _B(F); CFile = ToFD; CRecPtr = RecPtr; B_(F2, b); break; }
				}
			Arg = (KeyFldD*)Arg->Chain; KF = (KeyFldD*)KF->Chain;
		}
		CFile = ToFD; CRecPtr = RecPtr;
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
			CFile->NRecs = 0; SetUpdHandle(CFile->Handle); XFNotValid(); goto label1;
		}
		else { SetMsgPar(CFile->Name); RunErrorM(md, 821); }
	if (N < OldNRecs) { DecNRecs(OldNRecs - N); goto label1; }
	CRecPtr = GetRecSpace(); ZeroAllFlds(); SetDeletedFlag();
	IncNRecs(N - OldNRecs); for (longint i = OldNRecs + 1; i < N; i++) WriteRec(i);
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
		if ((F->Flg && f_Stored != 0) && (F->Typ == 'T')) DelTFld(F);
		F = (FieldDescr*)F->Chain;
	}
}

void CopyRecWithT(void* p1, void* p2)
{
	Move(p1, p2, CFile->RecLen);
	FieldDescr* F = CFile->FldD;
	while (F != nullptr) {
		if ((F->Typ == 'T') && (F->Flg && f_Stored != 0)) {
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

void RandDoubleByBytes(double& nr)
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
	BYTE* TX = (BYTE*)&T;
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
		MaxPage = *TNxtAvailPage - 1; GetMLen(); return;
	}
	if (Format == FptFormat) {
		FreePart = SwapLong((*FptHd).FreePart);
		BlockSize = Swap((*FptHd).BlockSize); return;
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
		RandDoubleByBytes(T.TimeStmp);
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
		RandDoubleByBytes(T.TimeStmp);
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
	BYTE* TX = (BYTE*)&T;
	longint* TNxtAvailPage = (longint*)&T;                               /* .DBT */
	struct stFptHd { longint FreePart; WORD X, BlockSize; }; /* .FPT */
	stFptHd* FptHd = (stFptHd*)&T;
	char Pw[40];
	// BYTE absolute 0 Time:0x46C; TODO: TIMER
	WORD i, n; BYTE sum; longint RS = 0;
	const PwCodeArr EmptyPw = { '@','@','@','@','@','@','@','@','@','@','@','@','@','@','@','@','@','@','@','@' };

	if (Format == DbtFormat) {
		//FillChar(&T, 512, ' '); 
		*TNxtAvailPage = MaxPage + 1; goto label1;
	}
	if (Format == FptFormat) {
		//FillChar(&T, 512, 0); 
		(*FptHd).FreePart = SwapLong(FreePart);
		(*FptHd).BlockSize = Swap(BlockSize); goto label1;
	}
	//FillChar(&T, 512, '@');
	Move(PwCode, Pw, 40); Code(Pw, 40); srand(RS);
	if (LicenseNr != 0) for (i = 1; i < 20; i++) Pw[i] = char(Random(255));
	n = 0x4000;
	// TODO: T.Time = Time;
	Move(Pw, T.PwNew, 40);
	srand(MLen + T.Time);
	for (i = 14; i < 511; i++) TX[i] = TX[i] ^ Random(255);
	T.LicNr = LicenseNr;
	if (LicenseNr != 0) {
		n = 0x6000; sum = T.LicNr;
		for (i = 1; i < 105; i++) sum = sum + T.LicText[i];
		T.Sum = sum;
	}
	Move(&FreePart, &T.FreePart, 23);
	T.OldMaxPage = 0xffff; T.Signum = 1; T.IRec += n;
	Move(&Version[1], &T.Version, 4);
	T.HasCoproc = HasCoproc;
	srand(RS);
label1:
	RdWrCache(false, Handle, NotCached(), 0, 512, &T);
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
	longint PosPg, NxtPg; WORD PosI; integer N; WORD l;
	BYTE X[MPageSize]; integer* XL = (integer*)&X;
	WORD* wp = nullptr; WORD* wpofs = wp; bool IsLongTxt;
	if (Pos <= 0) return;
	if ((Format != T00Format) || NotCached()) return;
	if ((Pos < MPageSize) || (Pos >= MLen)) { Err(889, false); return; }
	PosPg = Pos & (0xFFFFFFFF << MPageShft);
	PosI = Pos & (MPageSize - 1);
	RdWrCache(true, Handle, NotCached(), PosPg, MPageSize, X);
	wp = (WORD*)(&X[PosI]); l = *wp;
	if (l <= MPageSize - 2) {       /* small text on 1 page*/
		*wp = -integer(l); N = 0; wp = (WORD*)(&X);
		while (N < MPageSize - 2) {
			if (*wp > 0) { FillChar(&X[PosI + 2], l, 0); goto label1; }
			N += -(*wp) + 2; *wpofs += -(*wp) + 2;
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
		l = WORD(XL); if (l > MaxLStrLen + 1) {
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
	/*asm  les di,Self; xor ax,ax; mov dl,es:[di].FileD.UMode;
	 cmp dl,Shared; je @1; cmp dl,RdShared; jne @2;
@1:  cmp es:[di].FileD.LMode,ExclMode; je @2;
	 mov ax,1;
@2:  end;*/
	return true;
}

WORD FileD::GetNrKeys()
{
	KeyD* k; WORD n;
	n = 0; k = Keys;
	while (k != nullptr) { n++; k = k->Chain; }
	return n;
}


void XString::Clear()
{
	this->S.clean();
}

void XString::StoreReal(double R, KeyFldD* KF)
{
	BYTE A[20];
	const BYTE TabF[18] = { 1, 1, 2, 2, 3, 3, 4, 4, 4, 5, 5, 6, 6, 6, 7, 7, 8, 8 };
	auto X = KF->FldD;

	if (X->Typ == 'R' || X->Typ == 'D') {
		bool b = KF->Descend;
		if (R < 0) { b = !b; R = -R; }
		StoreD(&R, b);
		return;
	}
	if ((X->Flg & f_Comma) == 0) R = R * Power10[X->M];
	WORD n = X->L - 1;
	if (X->M > 0) n--;
	n = TabF[n];
	FixFromReal(R, A, n);
	StoreF(A, n, KF->Descend);
}

void XString::StoreStr(pstring V, KeyFldD* KF)
{
	WORD n;
	auto X = KF->FldD;
	while (V[0] < X->L) {
		if (X->M == LeftJust) V = V + " ";
		else {
			auto oldV = V;
			V = " ";
			V += oldV;
		}
	}
	if (X->Typ == 'N') {
		// Pack(&V[1], &V[0], X->L);
		n = (X->L + 1) / 2;
		StoreN(&V, n, KF->Descend);
	}
	else StoreA(&V[1], X->L, KF->CompLex, KF->Descend);
}

void XString::StoreBool(bool B, KeyFldD* KF)
{
	StoreN(&B, 1, KF->Descend);
}

void XString::StoreKF(KeyFldD* KF)
{
	FieldDPtr F;
	F = KF->FldD;
	switch (F->FrmlTyp) {
	case 'S': StoreStr(_ShortS(F), KF); break;
	case 'R': StoreReal(_R(F), KF); break;
	case 'B': StoreBool(_B(F), KF); break;
	}
}

void XString::PackKF(KeyFldD* KF)
{
	Clear();
	while (KF != nullptr) { StoreKF(KF); KF = (KeyFldD*)KF->Chain; }
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
}

void XString::StoreN(void* N, WORD Len, bool Descend)
{
}

void XString::StoreF(void* F, WORD Len, bool Descend)
{
}

void XString::StoreA(void* A, WORD Len, bool CompLex, bool Descend)
{
}

longint XItem::GetN()
{
	// asm les bx,Self; mov ax,es:[bx]; mov dl,es:[bx+2]; xor dh,dh end;
	return 0;
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

XItem* XItem::Next(WORD O)
{
	// asm les bx,Self; add bx,O; xor ah,ah; mov al,es:[bx+1]; add ax,bx; add ax,2;
	// mov dx, es
	return nullptr;
}

WORD XItem::UpdStr(WORD O, pstring* S)
{
	/*asm  push ds; lds bx,Self; les di,S; cld; add bx,O;
	 mov al,[bx];{M} add al,[bx+1];{L} stosb;
	 mov al,[bx]; xor ah,ah; add di,ax; lea si,[bx+2];
	 xor ch,ch; mov cl,[bx+1]; rep movsb; mov ax,si; pop ds;*/
	return 0;
}

WORD XPage::Off()
{
	if (IsLeaf) return oLeaf;
	else return oNotLeaf;
}

XItem* XPage::XI(WORD I)
{
	XItemPtr x; WORD o;
	x = XItemPtr(&A);
	o = Off();
	while (I > 1) { x = x->Next(o); I--; }
	return x;
}

uintptr_t XPage::EndOff()
{
	XItemPtr x = nullptr;
	WORD* xofs = (WORD*)x; // absolute x
	x = XI(NItems + 1); return uintptr_t(xofs);
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
	XItemPtr x = XItemPtr(&A);
	WORD* xofs = (WORD*)x; // absolute x
	WORD o = 0;
	pstring* s = nullptr;

	o = Off();
	//TODO: asm les di, @result; mov s.WORD, di; mov s[2].WORD, es;

	if (I > NItems) s[0] = 0;
	else {
		for (WORD j = 1; j < I; j++) { *xofs = x->UpdStr(o, s); }
	}
	//TODO: co a jak to vrac�?
	return "";
}

longint XPage::SumN()
{
	if (IsLeaf) { return NItems; }
	longint n = 0;
	XItemPtr x = XItemPtr(&A);
	WORD o = Off();
	for (WORD i = 1; i < NItems; i++) { n += x->GetN(); x = x->Next(o); }
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
	NItems++; x = XI(I);
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
	x->DownPage = Page;
}

void XPage::Delete(WORD I)
{
	XItemPtr x = nullptr, x1 = nullptr, x2 = nullptr;
	WORD* xofs = (WORD*)x;
	WORD* x1ofs = (WORD*)x1;
	WORD* x2ofs = (WORD*)x2;
	WORD o = Off(); WORD oE = EndOff(); x = XI(I);
	if (I < NItems) {
		x2 = x->Next(o);
		integer d = x2->GetM(o) - x->GetM(o);
		if (d <= 0) Move(x2, x, oE - *x2ofs);
		else {
			Move(x2, x, o);
			x->PutL(o, x2->GetL(o) + d); x1 = x;
			*x1ofs = *x1ofs + o + 2 + d;
			*x2ofs = *x2ofs + o + 2;
			Move(x2, x1, oE - *x2ofs);
		}
		x = XI(NItems);
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
	XItemPtr xE = XI(NItems + 1);
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
	// figuruje tady pstring* s, ale v�sledek se nikam neukl�d�, je to zakomentovan�

	XItemPtr x = nullptr, x1 = nullptr, x2 = nullptr;
	WORD* xofs = (WORD*)x;
	WORD* x1ofs = (WORD*)x1;
	WORD* x2ofs = (WORD*)x2;
	WORD o, oA, oE, n;
	pstring* s;

	x = XItemPtr(&A); x1 = x; o = Off(); oA = *xofs; oE = EndOff(); n = 0;
	while (*xofs - oA < oE - *xofs + x->GetM(o)) { x = x->Next(o); n++; }
	FillChar(P, XPageSize, 0);
	Move(x1, P->A, *xofs - oA);
	//s = (pstring*)(uintptr_t(x1) + oA + o + 1);;
	//s = &StrI(n + 1);
	Move(x, x1, o);
	x1->PutM(o, 0);
	x1 = x1->Next(o); x = x->Next(o); Move(x, x1, oE - *xofs);
	P->NItems = n; NItems -= n; *xofs = EndOff(); FillChar(x, oE - *xofs, 0);
	if (IsLeaf) P->GreaterPage = ThisPage; else P->GreaterPage = 0;
	P->IsLeaf = IsLeaf;
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
	XPagePtr p;
	WORD iItem = 0;
	char result;
	{
		p = new XPage(); // (XPage*)GetStore(XPageSize);
	label1:
		XPathN = 1; 
		longint page = IndexRoot; 
		AfterEqu = AfterEqu && Duplic;
		XPath[XPathN].Page = page;
		XF()->RdPage(p, page);
		XItemPtr x = XItemPtr(p->A);
		WORD o = p->Off();
		WORD nItems = p->NItems;
		if (nItems == 0) {
			RecNr = CFile->NRecs + 1;
			XPath[1].I = 1;
			goto label2;
		}

		//__asm {
		//	push ds;
		//	cld;
		//	les dx, x;
		//	mov iItem, 1;
		//}

		//asm
		//	push ds; cld; les bx, x; mov iItem, 1; mov dx, 1;
		//@@add 1 bx, o;xor ax, ax; mov al, es: [bx] ; cmp dx, ax; jna @@5; /*first different <= prefix length?*/
		//mov dx, ax; lds si, XX;xor ax, ax; lodsb; sub ax, dx; add si, dx;
		//mov ah, es: [bx + 1] ; /*pstring length*/ lea di, [bx + 2]; /*pstring addr*/
		//xor cx, cx; mov cl, ah; cmp ah, al; jna @@2; mov cl, al;  /*min length*/
		//@@add 2 dx, cx;xor ch, ch; /*set zero flag*/
		//repe cmpsb; jb @@8; ja @@4; cmp al, ah; jb @@8; ja @@3;
		//cmp AfterEqu, 0; je @@7;
		//@@inc 3 dx;
		//@@sub 4 dx, cx;
		//@@mov 5 ax, iItem; cmp ax, nItems; je @@6; /*last item?*/
		//inc ax; mov iItem, ax;
		//xor ax, ax; mov al, es: [bx + 1] ; add ax, 2; add bx, ax;  /*next item*/
		//jmp @@1;
		//@@mov 6 al, _gt; inc iItem; jmp @@9;
		//@@mov 7 al, _equ; jmp @@9;
		//@@mov 8 al, _lt;
		//@@mov 9 result, al; sub bx, o; mov x.WORD, bx; pop ds; }
		XPath[XPathN].I = iItem;
		if (p->IsLeaf) {
			if (iItem > nItems) RecNr = CFile->NRecs + 1; else RecNr = x->GetN();
			if (searchResult == _equ)
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
			return searchResult;
		}
		if (iItem > nItems) page = p->GreaterPage;
		else page = x->DownPage;
		XPathN++;
		goto label1;
	}
	return searchResult;
}

bool XKey::SearchIntvl(XString& XX, bool AfterEqu, longint& RecNr)
{
	return Search(XX, AfterEqu, RecNr) || Intervaltest && (RecNr <= CFile->NRecs);
}

longint XKey::PathToNr()
{
	WORD i, j; longint n; XPagePtr p; XItemPtr x;
	p = (XPage*)GetStore(XPageSize); n = 0;
	for (j = 1; j < XPathN - 1; j++)
	{
		XF()->RdPage(p, XPath[j].Page);
		x = XItemPtr(p->A);
		for (i = 1; i < XPath[j].I - 1; i++) { (n += x->GetN()); x = x->Next(oNotLeaf); };
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
	XItemPtr x = XItemPtr(p->A);
	for (WORD j = 1; j < p->NItems; j++) {
		if (I <= x->GetN()) { 
			XPath[XPathN].I = j; 
			page = x->DownPage; 
			goto label1; }
		I -= x->GetN();
		x = x->Next(oNotLeaf);
	}
	XPath[XPathN].I = p->NItems + 1;
	page = p->GreaterPage;
	goto label1;
}

longint XKey::PathToRecNr()
{
	auto X = XPath[XPathN];
	XPagePtr p = (XPage*)GetStore(XPageSize); /* !!! with XPath[XPathN] do!!! */
	XF()->RdPage(p, X.Page);
	longint recnr = p->XI(X.I)->GetN();
	longint result = recnr;
	if ((recnr == 0) || (recnr > CFile->NRecs)) XF()->Err(835);
	ReleaseStore(p);
	return result;
}

bool XKey::RecNrToPath(XString& XX, longint RecNr)
{
	bool result;
	XPagePtr p; XItemPtr x; longint n;
	XX.PackKF(KFlds); Search(XX, false, n);
	p = (XPage*)GetStore(XPageSize);
	result = false; /* !!! with XPath[XPathN] do!!! */
	{
		auto X = XPath[XPathN];
	label1:
		XF()->RdPage(p, X.Page);
		x = p->XI(X.I);
		if (!(p->StrI(X.I) == XX.S)) goto label3;
	label2:
		if (x->GetN() == RecNr) { result = true; goto label3; }
		X.I++;
		if (X.I > p->NItems) {
			if (IncPath(XPathN - 1, X.Page)) { X.I = 1; goto label1; }
		}
		else {
			x = x->Next(oLeaf); if (x->GetL(oLeaf) != 0) goto label3; goto label2;
		}; }
label3:
	ReleaseStore(p);
	return result;
}

bool XKey::IncPath(WORD J, longint& Pg)
{
	bool result;
	XPagePtr p;
	p = (XPage*)GetStore(XPageSize);
	result = false;
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
		else Pg = p->XI(X.I)->DownPage;
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
	XPagePtr p = (XPage*)GetStore(XPageSize); NrToPath(I); /* !!! with XPath[XPathN] do!!! */
	XF()->RdPage(p, XPath[XPathN].Page);
	result = p->StrI(I);
	ReleaseStore(p);
	return result;
}

longint XKey::RecNrToNr(longint RecNr)
{
	XString x;
	if (RecNrToPath(x, RecNr)) return PathToNr(); else return 0;
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
	WORD i, j;
	longint page, page1, uppage, downpage;
	XItemPtr x;
	longint n, upsum;
	XPagePtr p, p1, upp;

	p = (XPage*)GetStore(2 * XPageSize);
	p1 = (XPage*)GetStore(2 * XPageSize);
	upp = (XPage*)GetStore(2 * XPageSize);
	for (j = XPathN; j > 1; j--) {
		page = XPath[j].Page; XF()->RdPage(p, page); i = XPath[j].I;
		if (p->IsLeaf) {
			InsertItem(XX, p, upp, page, i, x, uppage); x->PutN(RecNr);
		}
		else {
			if (i <= p->NItems) {
				x = p->XI(i); n = x->GetN() + 1; if (uppage != 0) n -= upsum;
				x->PutN(n);
			}
			if (uppage != 0) {
				downpage = uppage; InsertItem(XX, p, upp, page, i, x, uppage);
				x->DownPage = downpage; x->PutN(upsum);
			};
		}
		XF()->WrPage(p, page); if (uppage != 0) {
			XF()->WrPage(upp, uppage); upsum = upp->SumN();
			if (upp->IsLeaf) ChainPrevLeaf(p1, uppage);
		}
	}
	if (uppage != 0) {
		page1 = XF()->NewPage(p1); p1->GreaterPage = page1;
		p1->InsDownIndex(1, uppage, upp); XF()->WrPage(p, page1); XF()->WrPage(p1, page);
		if (upp->IsLeaf) {
			upp->GreaterPage = page1; XF()->WrPage(upp, uppage);
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
		if (I <= UpP->NItems) X = UpP->XI(I);
		else X = P->XI(I - UpP->NItems);
		XX.S = UpP->StrI(UpP->NItems);
	}
}

void XKey::ChainPrevLeaf(XPagePtr P, longint N)
{
	longint page; WORD i, j;
	for (j = XPathN - 1; j > 1; j--)
		if (XPath[j].I > 1) {
			XF()->RdPage(P, XPath[j].Page); i = XPath[j].I - 1;
		label1:
			page = P->XI(i)->DownPage; XF()->RdPage(P, page);
			if (P->IsLeaf) { P->GreaterPage = N; XF()->WrPage(P, page); return; }
			i = P->NItems; goto label1;
		}
}

bool XKey::Insert(longint RecNr, bool Try)
{
	longint N, XNr; XString x;
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
	else Page1 = P->XI(I)->DownPage;
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
		IndexLen += KF->FldD->NBytes;
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
	XPagePtr p = (XPage*)GetStore(XPageSize);
	XF()->RdPage(p, Page);
	if (!p->IsLeaf) {
		WORD n = p->NItems;
		for (WORD i = 1; i < n; i++) {
			ReleaseTree(p->XI(i)->DownPage, IsClose);
			XF()->RdPage(p, Page);
		}
		if (p->GreaterPage != 0) ReleaseTree(p->GreaterPage, IsClose);
	}
	if ((Page != IndexRoot) || IsClose)
		XF()->ReleasePage(p, Page);
	else {
		FillChar(p, XPageSize, 0); p->IsLeaf = true; XF()->WrPage(p, Page);
	}
	ReleaseStore(p);
}

void XWKey::OneRecIdx(KeyFldD* KF, longint N)
{
	Open(KF, true, false); Insert(N, true); NR++;
}

void XWKey::InsertAtNr(longint I, longint RecNr)
{
	XString x;
	x.PackKF(KFlds); NR++; NrToPath(I); InsertOnPath(x, RecNr);
}

longint XWKey::InsertGetNr(longint RecNr)
{
	XString x; longint n;
	NR++; x.PackKF(KFlds); Search(x, true, n);
	auto result = PathToNr();
	InsertOnPath(x, RecNr);
	return result;
}

void XWKey::DeleteAtNr(longint I)
{
	NrToPath(I); DeleteOnPath(); NR--;
}

void XWKey::AddToRecNr(longint RecNr, integer Dif)
{
	if (NRecs() == 0) return;
	NrToPath(1);
	XPagePtr p = (XPage*)GetStore(sizeof(*p)); /* !!! with XPath[XPathN] do!!! */
	longint pg = XPath[XPathN].Page;
	integer j = XPath[XPathN].I;
	do {
		XF()->RdPage(p, pg);
		integer n = p->NItems - j + 1;
		XItemPtr x = p->XI(j);
		while (n > 0) {
			longint nn = x->GetN();
			if (nn >= RecNr) x->PutN(nn + Dif);
			x = x->Next(oLeaf); n--;
		}
		XF()->WrPage(p, pg); pg = p->GreaterPage; j = 1;
	} while (pg != 0);
	ReleaseStore(p);
}

void XWFile::Err(WORD N)
{
	if (this == &XWork) { SetMsgPar(FandWorkXName); RunError(N); }
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

void XWFile::RdPage(XPagePtr P, longint N)
{
	if ((N == 0) || (N > MaxPage)) Err(831);
	RdWrCache(true, Handle, NotCached(), N << XPageShft, XPageSize, P);
}

void XWFile::WrPage(XPagePtr P, longint N)
{
	if (UpdLockCnt > 0) Err(645);
	RdWrCache(false, Handle, NotCached(), N << XPageShft, XPageSize, P);
}

longint XWFile::NewPage(XPagePtr P)
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
	FillChar(P, XPageSize, 0);
	return result;
}

void XWFile::ReleasePage(XPagePtr P, longint N)
{
	FillChar(P, XPageSize, 0);
	P->GreaterPage = FreeRoot;
	FreeRoot = N;
	WrPage(P, N);
}

void XFile::SetEmpty()
{
	auto p = (XPage*)GetZStore(XPageSize);
	WrPage(p, 0);
	p->IsLeaf = true; FreeRoot = 0; NRecs = 0;
	KeyDPtr k = CFile->Keys;
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
	RdWrCache(true, Handle, NotCached(), 2, 18, &FreeRoot);
}

void XFile::WrPrefix()
{
	WORD Signum = 0x04FF;
	RdWrCache(false, Handle, NotCached(), 0, 2, &Signum);
	NRecsAbs = CFile->NRecs; NrKeys = CFile->GetNrKeys();
	RdWrCache(false, Handle, NotCached(), 2, 18, &FreeRoot);
}

void XFile::SetNotValid()
{
	NotValid = true; MaxPage = 0; WrPrefix(); SaveCache(0);
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
		//P = (XPage*)GetStore(XPageSize);
		P = new XPage();
		Kind = 1;
		if (aKIRoot != nullptr) Kind = 2;
	}
}

void XScan::Reset(FrmlPtr ABool, bool SQLFilter)
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
			a1.S = *k->X1;
			b2.S = *k->X2;
			Key->FindNr(a1, k->XNrBeg);
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

void XScan::ResetSort(KeyFldDPtr aSK, FrmlPtr& BoolZ, LockMode OldMd, bool SQLFilter)
{
	LockMode m;
	if (Kind == 4) {
		SK = aSK;
		if (SQLFilter) { Reset(BoolZ, true); BoolZ = nullptr; }
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
	Key = WK; if (Kind != 3) Kind = 1;
	if (P == nullptr) P = (XPage*)GetStore(XPageSize);
	NRecs = Key->NRecs();
	Bool = nullptr; SeekRec(0); TempWX = true;
}

void XScan::ResetOwner(XString* XX, FrmlPtr aBool)
{
	longint n; bool b;
	CFile = FD; Bool = aBool;
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
		KIRoot = (KeyInD*)GetZStore(sizeof(*KIRoot));
		Key->FindNr(*XX, KIRoot->XNrBeg);
		AddFFs(Key, XX->S);
		b = Key->FindNr(*XX, n); NRecs = n - KIRoot->XNrBeg + b;
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
		KF1 = (KeyFldD*)KF1->Chain; KF2 = (KeyFldD*)KF2->Chain;
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
	KeyInD* k; FrmlPtr z;
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
	if (!eof)
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
			KI = k; SeekOnKI(I);
			break;
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

void CmpLxStr()
{
}

WORD CompLexLongStr(LongStrPtr S1, LongStrPtr S2)
{
	return 0;
}

WORD CompLexLongShortStr(LongStrPtr S1, pstring& S2)
{
	return 0;
}

WORD CompLexStr(const pstring& S1, const pstring& S2)
{
	return 0;
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
		case 2: { RecNr = X->GetN(); NOnPg--;
			if (NOnPg > 0) X = X->Next(oLeaf);
			else if ((Kind == 2) && (NOfKI == 0)) NextIntvl();
			else if (P->GreaterPage > 0) SeekOnPage(P->GreaterPage, 1);
		label2:
			ReadRec(RecNr); if (DeletedFlag()) goto label1;
		label3:
			if (!RunBool(Bool)) goto label1;
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
	Key->XF()->RdPage(P, Page); NOnPg = P->NItems - I + 1;
	if (Kind == 2)
	{
		if (NOnPg > NOfKI) NOnPg = NOfKI;
		NOfKI -= NOnPg;
	}
	X = P->XI(I);
}

void XScan::NextIntvl()
{
	XString xx; bool b; longint n, nBeg; WKeyDPtr k;
	if (OwnerLV != nullptr) {
		k = WKeyDPtr(OwnerLV->RecPtr);
		while (iOKey < k->NRecs()) {
			iOKey++;
			CFile = OwnerLV->FD; xx.S = k->NrToStr(iOKey);
			CFile = FD;
			Key->FindNr(xx, nBeg); AddFFs(Key, xx.S);
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
		do { KI = (KeyInD*)KI->Chain; } while ((KI != nullptr) || (KI->N > 0));
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
}

bool HasTWorkFlag()
{
	return false;
}

void SetUpdFlag()
{
}

void ClearUpdFlag()
{
}

bool HasUpdFlag()
{
	return false;
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
	else XDecode(S); // mus� m�t o 2B v�c - sah� si tm XDecode!!!
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

