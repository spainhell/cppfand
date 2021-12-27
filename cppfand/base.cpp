#pragma once

#include "base.h"
#include <windows.h>
#include <errhandlingapi.h>
#include <fileapi.h>
#include "OldDrivers.h"
#include "legacy.h"
#include <set>
#include <vector>
#include "obaseww.h"
#include "oaccess.h"
#include <ctime>
#include <iostream>

#include "compile.h"
#include "FileD.h"
#include "GlobalVariables.h"
#include "../datafiles/datafiles.h"
#include "../exprcmp/exprcmp.h"
#include "../fandio/cache.h"
#include "../Logging/Logging.h"


Video video;
Spec spec;
Fonts fonts;
Colors colors;

char CharOrdTab[256];
char UpcCharTab[256];
WORD TxtCols = 80;
WORD TxtRows = 25;

integer prCurr, prMax;

wdaystt WDaysTabType;
WORD NWDaysTab;
double WDaysFirst;
double WDaysLast;
wdaystt* WDaysTab;

char AbbrYes = 'Y';
char AbbrNo = 'N';

WORD HandleError;
pstring OldDir;
pstring FandDir;
std::string WrkDir;
pstring FandOvrName;
pstring FandResName;
pstring FandWorkName;
pstring FandWorkXName;
pstring FandWorkTName;
std::string CPath;
std::string CDir;
std::string CName;
std::string CExt;
std::string CVol;

TResFile ResFile;
TMsgIdxItem* MsgIdx;// = TMsgIdx;
WORD MsgIdxN;
longint FrstMsgPos;

WORD F10SpecKey; // r. 293
BYTE ProcAttr;
// bool SetStyleAttr(char c, BYTE& a); // je v KBDWW
std::string MsgLine;
std::string MsgPar[4];

WORD OldNumH; // r1 
void* OldHTPtr = nullptr;

//#ifdef FandDemo
//WORD files = 30;
//#else
//WORD files = 250; // {files in CONFIG.SYS -3}
//#endif
//WORD CardHandles;

Cache cache;
//std::map<FILE*, FileCache*> Cache::cacheMap;
WORD CachePageSize;
void* AfterCatFD; // r108
ProcStkD* MyBP;
ProcStkD* ProcMyBP;
WORD BPBound; // r212
bool ExitP, BreakP;
longint LastExitCode = 0; // r215
bool WasLPTCancel;
FILE* WorkHandle;
longint MaxWSize = 0; // {currently occupied in FANDWORK.$$$}
void* FandInt3f; // ø. 311
FILE* OvrHandle;
WORD Fand_ss, Fand_sp, Fand_bp, DML_ss, DML_sp, DML_bp;
longint _CallDMLAddr = 0; // {passed to FANDDML by setting "DMLADDR="in env.}
Printer printer[10];
TPrTimeOut OldPrTimeOut;
TPrTimeOut PrTimeOut;  // absolute 0:$478;
bool WasInitDrivers = false;
bool WasInitPgm = false;
WORD LANNode; // ø. 431
void (*CallOpenFandFiles)(); // r453
void (*CallCloseFandFiles)(); // r454

double userToday = 0;
ExitRecord ExitBuf;

typedef FILE* filePtr;

std::set<FILE*> Handles;
std::set<FILE*> UpdHandles;
std::set<FILE*> FlshHandles;

//map<WORD, FILE*> fileMap;
// náhrada za 'WORD OvrHandle = h - 1' - zjištìní pøedchozího otevøeného souboru;
std::vector<FILE*> vOverHandle;

void SetMsgPar(std::string s)
{
	MsgPar[0] = s;
}

void SetMsgPar(std::string s1, std::string s2)
{
	MsgPar[0] = s1;
	MsgPar[1] = s2;
}

void SetMsgPar(std::string s1, std::string s2, std::string s3)
{
	SetMsgPar(s1, s2);
	MsgPar[2] = s3;
}

void SetMsgPar(std::string s1, std::string s2, std::string s3, std::string s4)
{
	SetMsgPar(s1, s2, s3);
	MsgPar[3] = s4;
}

longint PosH(FILE* handle)
{
	if (handle == nullptr) return -1;
	try
	{
		const auto result = ftell(handle);
		HandleError = ferror(handle);
		return static_cast<longint>(result);
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << "\n";
		return -1;
	}
}

longint MoveH(longint dist, WORD method, FILE* handle)
{
	if (handle == nullptr) return -1;
	// dist - hodnota offsetu
	// method: 0 - od zacatku, 1 - od aktualni, 2 - od konce
	// handle - file handle
	try
	{
		auto result = fseek(handle, dist, method);
		if (result != 0) {
			errno_t err;
			_get_errno(&err);
			HandleError = err;
			return -1;
		}
		HandleError = (WORD)result;
		return ftell(handle);
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << "\n";
		return -1;
	}
}

int SeekH(FILE* handle, longint pos)
{
	if (handle == nullptr) RunError(705);
	return MoveH(pos, 0, handle);
}

size_t ReadH(FILE* handle, size_t bytes, void* buffer)
{
	//if (CFile != nullptr && CFile->Name == "DEALER")
	//{
	//	printf("ReadH() r160: DEALER\n");
	//}
	//if ((uintptr_t)handle == 0x0117d388)
	//{
	//	printf("");
	//}
	return fread_s(buffer, bytes, 1, bytes, handle);
}

void RdMsg(integer N)
{
	WORD j, o;
	FILE* h;
	pstring s;
	for (int i = 1; i < MsgIdxN; i++) {
		auto Nr = MsgIdx[i].Nr;
		auto Count = MsgIdx[i].Count;
		auto Ofs = MsgIdx[i].Ofs;
		if (N >= Nr && N < Nr + Count)
		{
			j = N - Nr + 1;
			o = Ofs;
			goto label1;
		}
	}
	o = 0; j = 1;
	MsgPar[1] = std::to_string(N).c_str();

label1:
	h = ResFile.Handle;
	SeekH(h, FrstMsgPos + o);

	for (int i = 1; i <= j; i++)
	{
		ReadH(h, 1, &s[0]); // tady se ma zrejme jen vycist delka
		ReadH(h, s.length(), &s[1]);
	}
	ConvKamenToCurr(&s[1], s.length());
	MsgLine = "";
	j = 0;
	s[s.length() + 1] = 0x00;
	for (int i = 1; i <= s.length(); i++) {
		if (s[i] == '$' && s[i + 1] != '$')
		{
			MsgLine += MsgPar[j];
			j++;
		}
		else
		{
			MsgLine += s[i];
			if (s[i] == '$') i++;
		}
	}

}

void WriteMsg(WORD N)
{
}

void ClearLL(BYTE attr)
{
	screen.ScrClr(1, TxtRows, TxtCols, 1, ' ', colors.uNorm);
}

WORD TResFile::Get(WORD Kod, void** P)
{
	WORD l = A[Kod].Size;
	//GetMem(P, l);
	*P = new BYTE(l);
	auto sizeF = FileSizeH(Handle);
	auto seekF = SeekH(Handle, A[Kod - 1].Pos);
	auto readF = ReadH(Handle, l, *P);
	return l;
}

std::string TResFile::Get(WORD Kod)
{
	char* tmpCh = new char[A[Kod].Size];
	WORD l = A[Kod].Size;

	auto sizeF = FileSizeH(Handle);
	auto seekF = SeekH(Handle, A[Kod].Pos);
	auto readF = ReadH(Handle, l, tmpCh);
	std::string result = std::string(tmpCh, l);
	delete[] tmpCh;
	return result;
}

LongStr* TResFile::GetStr(WORD Kod)
{
	LongStr* s;
	/* !!! with A[Kod] do!!! */
	s = (LongStr*)GetStore(A[Kod].Size + 2);
	s->LL = A[Kod].Size;
	SeekH(Handle, A[Kod].Pos);
	ReadH(Handle, A[Kod].Size, s->A);
	return s;
}

void* GetStore(WORD Size)
{
	return nullptr;
}

void* GetZStore(WORD Size)
{
	return nullptr;
}

WORD StackOvr()
{
	/*
	 procedure StackOvr(NewBP:word);
	type SF=record BP:word; case byte of 0:(Ret:pointer); 1:(RetOfs:word) end;
	var p,q:^SF;   pofs:word absolute p; qofs:word absolute q;
	r,t:^word; rofs:word absolute r; tofs:word absolute t;
	label 1;
	begin
	asm mov p.word,bp; mov p[2].word,ss end; pofs:=p^.BP;
	while pofs<NewBP do begin r:=p^.ret; pofs:=p^.BP;
	if (rofs=0) and (r^=$3fcd) then begin
	q:=ptr(SSeg,NewBP); inc(rofs,2);
	while qofs<BPBound do begin t:=q^.ret;
	if (seg(t^)=seg(r^)) and (tofs<>0) then begin
	   r^:=tofs; q^.retofs:=0; goto 1 end;
	qofs:=q^.BP end end;
	1:end;
	end;
	 */
	return 0;
}

void NoOvr()
{
	/*
		procedure NoOvr; far; assembler;
		asm   pop ax; pop ax; pop ax{bp}; push ax; push ax; call StackOvr;
			  pop bp; pop ds; pop ax; pop dx; pop sp; push dx; push ax;
		end;
	 */
}

bool CacheLocked = false; // r510

void AddBackSlash(std::string& s)
{
	if (s.empty()) { return; }
	if (s[s.length() - 1] == '\\' || s[s.length() - 1] == '/') return;

	if (s.find('\\') != std::string::npos) {
		s += '\\';
	}
	else {
		s += '/';
	}
}

void DelBackSlash(std::string& s)
{
	if (s.empty()) return;
	if (s[s.length() - 1] == '\\' || s[s.length() - 1] == '/') {
		s.erase(s.length() - 1, 1);
	}
}

pstring StrPas(const char* Src)
{
	WORD n = 0;
	pstring s;
	while ((n < 255) && (Src[n] != '\0')) { s[n + 1] = Src[n]; n++; }
	s[0] = (char)n;
	return s;
}

void ChainLast(Chained* Frst, Chained* New)
{
	if (Frst == nullptr) throw std::exception("ChainLast: Parent is NULL!");

	if (New == nullptr) return;

	Chained* last = Frst;

	while (last->Chain != nullptr) {
		last = last->Chain;
	}

	last->Chain = New;

	New->Chain = nullptr; // TODO: pridano kvuli zacykleni v RdAutoSortSK_M
}

Chained* LastInChain(Chained* Frst)
{
	Chained* last = Frst->Chain;
	if (last == nullptr) {
		return Frst;
	}
	while (last != nullptr) {
		if (last->Chain == nullptr) return last;
		last = last->Chain;
	}
	return last;
}

void StrLPCopy(char* Dest, pstring s, WORD MaxL)
{
	auto sLen = s.length();
	int len = (sLen < MaxL) ? sLen : MaxL;
	Move((void*)s.c_str(), Dest, len);
}

integer MinI(integer X, integer Y)
{
	if (X < Y) return X;
	return Y;
}

integer MaxI(integer X, integer Y)
{
	if (X > Y) return X;
	return Y;
}

WORD MinW(WORD X, WORD Y)
{
	if (X < Y) return X;
	return Y;
}

WORD MaxW(WORD X, WORD Y)
{
	if (X > Y) return X;
	return Y;
}

longint MinL(longint X, longint Y)
{
	if (X < Y) return X;
	return Y;
}

longint MaxL(longint X, longint Y)
{
	if (X > Y) return X;
	return Y;
}

bool OlympYear(WORD year)
{
	return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
}

WORD OlympYears(WORD year)
{
	if (year < 3) return 0;
	year--;
	return year / 4 - year / 100 + year / 400;
}

void SplitDate(double R, WORD& d, WORD& m, WORD& y)
{
	WORD i, j;
	longint l = (longint)trunc(R);

	if (l == 0) { y = 1; m = 1; d = 1; }
	else {
		y = l / 365; y++; l = l % 365;
		while (l <= OlympYears(y)) { y--; l += 365; }
		l = l - OlympYears(y);
		for (j = 1; j <= 12; j++) {
			i = NoDayInMonth[j];
			if ((j == 2) && OlympYear(y)) i++;
			if (i >= l) goto label1;
			l -= i;
		}
	label1:
		m = j; d = l;
	}
}

void wait()
{
}

bool MouseInRect(WORD X, WORD Y, WORD XSize, WORD Size)
{
	return false;
}

void EncodeMask(pstring& Mask, WORD& Min, WORD& Max)
{
	char Code[] = "YMDhmst";
	WORD i = 0, j = 0;
	Min = 9; Max = 0;
	for (i = 1; i <= Mask.length(); i++)
	{
		for (j = 0; j < 7; j++) {
			if ((char)Mask[i] == Code[j])
			{
				Mask[i] = (char)j;
				if (Min > j) Min = j;
				if (Max < j) Max = j;
			}
		}
	}
}

void AnalDateMask(pstring& Mask, WORD& I, WORD& IDate, WORD& N)
{
	N = 0;
	if (Mask[I] <= 6) {
		IDate = Mask[I];
		do { I++; N++; } while (!((I > Mask.length()) || (Mask[I] != IDate)));
	}
}

double RDate(WORD Y, WORD M, WORD D, WORD hh, WORD mm, WORD ss, WORD tt)
{
	WORD i = 0; longint l = 0, n = 0; double r = 0;
	if ((D > NoDayInMonth[M]) && ((M != 2) || (D != 29) || !(OlympYear(Y)))) { return 0; }
	if (Y + M + D == 0) l = 0;
	else {
		l = longint(Y - 1) * 365 + OlympYears(Y) + D;
		for (i = 1; i <= M - 1; i++) l = l + NoDayInMonth[i];
		if ((M > 2) && OlympYear(Y)) l++;
	}
	n = tt + 100 * ss + 6000 * longint(mm); r = (n + 360000.0 * hh) / (8640000.0);
	return l + r;
}

double Today()
{
	std::time_t t = std::time(0);   // get time now
	struct tm lt;
	errno_t err = localtime_s(&lt, &t);
	return RDate(lt.tm_year + 1900, lt.tm_mon + 1, lt.tm_mday, 0, 0, 0, 0);
}

std::string CppToday()
{
	std::time_t t = std::time(0);   // get time now
	struct tm lt;
	char buffer[32]{ '\0' };
	errno_t err = localtime_s(&lt, &t);
	strftime(buffer, sizeof(buffer), "%d.%m.%Y", &lt);
	return buffer;
}

double CurrTime()
{
	std::time_t t = std::time(0);   // get time now
	struct tm lt;
	errno_t err = localtime_s(&lt, &t);
	//return RDate(1 + 1900, 1, 0, lt.tm_hour, lt.tm_min, lt.tm_sec, 0);
	return RDate(1, 1, 0, lt.tm_hour, lt.tm_min, lt.tm_sec, 0);
}

double ValDate(pstring Txt, pstring Mask)
{
	struct Z { longint Y = 0, M = 0, D = 0, hh = 0, mm = 0, ss = 0, tt = 0; } z;
	longint* Date = &z.Y;
	WORD i = 0, j = 0, k = 0, min = 0, max = 0, iDate = 0,
		n = 0, Ylength = 0, Year = 0, y = 0, Month = 0, Day = 0;
	pstring s; bool WasYMD = false, WasMinus = false; double R = 0; longint nl = 0;

	double result = 0.0;
	Ylength = 0; z.Y = -1; z.M = -1; z.D = -1;
	for (i = 3; i <= 6; i++) Date[i] = 0;
	WasYMD = false; WasMinus = false;
	EncodeMask(Mask, min, max);
	i = 1; j = 1;
label1:
	if (j > Txt.length()) goto label2;
	else if (i > Mask.length()) return result;
	AnalDateMask(Mask, i, iDate, n);
	if (n == 0) {
		if (Mask[i] != Txt[j]) return result;
		i++; j++;
	} /* delimiter */
	else { /* YMDhmst */
		s = "";
		if (iDate < 3) WasYMD = true;
		while ((Txt[j] == ' ') && (n > 1)) { j++; n--; }
		if ((Txt[j] == '-') && (n > 1) && (iDate == min) && (iDate > 2)) {
			WasMinus = true; j++; n--;
		}
		if (!(Txt[j] >= '0' && Txt[j] <= '9')) return result;
		while ((j <= Txt.length()) && (isdigit(Txt[j])) && (n > 0)) {
			s.Append(Txt[j]); j++; n--;
		}
		val(s, Date[iDate], k);
		if (iDate == 0) Ylength = s.length();
	}
	goto label1;
label2:
	if ((min == 2) && (max >= 3)) {
		if (z.D < 0) z.D = 0;
		R = z.D + (z.tt + 100 * z.ss + 6000 * longint(z.mm) + 360000.0 * z.hh) / 8640000.0;
		goto label3;
	}
	if (WasYMD) {
		SplitDate(Today(), Day, Month, Year);
		/*if ((max<3) && (z.D=-1) && (z.M=-1) && (z.Y=-1)) return;*/
		if (z.D == -1) z.D = 1;
		else {
			if ((z.D == 0) || (z.D > 31)) return result;
			else if (z.M == -1) z.M = Month;
		}
		if (z.M == -1) z.M = 1;
		else if ((z.M == 0) || (z.M > 12)) return result;
		if (Ylength == 0) z.Y = Year;
		else if (z.Y > 9999) return result;
		else
			if (Ylength <= 2) {
				if (spec.OffDefaultYear == 0) z.Y = (Year / 100) * 100 + z.Y;
				else {
					y = (Year + spec.OffDefaultYear) % 100;
					if (z.Y < y) z.Y = z.Y + 2000; else z.Y = z.Y + 1900;
				}
			}
	}
	else { z.Y = 0; z.M = 0; z.D = 0; }
	if ((min < 3) && (z.hh > 23)) return result;
	if ((min < 4) && (z.mm > 59)) return result;
	if ((min < 5) && (z.ss > 59)) return result;
	R = RDate(z.Y, z.M, z.D, z.hh, z.mm, z.ss, z.tt);
label3:
	if (!WasYMD && (R == 0.0)) R = 1E-11;
	if (WasMinus) result = -R;
	else result = R;
	return result;
}

bool ExCode(WORD N, pstring Mask)
{
	for (WORD i = 1; i <= Mask.length(); i++) {
		if ((Mask[i]) == N) return true;
	}
	return false;
}

pstring StrDate(double R, pstring Mask)
{
	struct stD { WORD Y = 0; WORD M = 0; WORD D = 0; } d;
	struct stT { longint hh = 0, mm = 0, ss = 0, tt = 0; } t;
	pstring x;
	WORD i = 0, iDate = 0, n = 0, m = 0, min = 0, max = 0;
	longint f = 0, l = 0; bool First = false, WasMinus = false; char c = 0;

	double MultX[7]{ 0, 0, 0, 24, 1440, 86400, 8640000 };
	longint DivX[12]{ 0, 0, 0, 1, 60, 3600, 360000, 1, 60, 6000, 1, 100 };

	pstring s = "";
	EncodeMask(Mask, min, max);
	WasMinus = false;
	if ((R == 0.0) || (R < 0) && (min < 3)) {
		for (i = 1; i <= Mask.length(); i++) {
			if (Mask[i] <= 6) s.Append(' ');
			else s.Append(Mask[i]);
		}
		goto label1;
	}
	else if (R < 0) { WasMinus = true; R = -R; }
	if (min < 3) {
		if ((min == 2) && (max >= 3)) d.D = trunc(R);
		else SplitDate(R, d.D, d.M, d.Y);
		double intpart;
		R = modf(R, &intpart);
	}
	if (max >= 3)
	{
		l = round(R * MultX[max]);
		if (ExCode(3, Mask))
		{
			f = DivX[max]; t.hh = l / f; l = l % f;
		}
		if (ExCode(4, Mask))
		{
			f = DivX[max + 3]; t.mm = l / f; l = l % f;
		}
		if (ExCode(5, Mask))
		{
			f = DivX[max + 5]; t.ss = l / f; l = l % f;
		}
		t.tt = l;
	}
	i = 1;
	First = true;
	while (i <= Mask.length())
	{
		AnalDateMask(Mask, i, iDate, n);
		if (n == 0) { s.Append(Mask[i]); i++; }
		else {
			if (iDate < 3) {
				if (iDate == 0)	str(d.Y, x);
				if (iDate == 1)	str(d.M, x);
				if (iDate == 2)	str(d.D, x);
			}
			else {
				if (iDate == 3) str(t.hh, x);
				if (iDate == 4) str(t.mm, x);
				if (iDate == 5) str(t.ss, x);
				if (iDate == 6) str(t.tt, x);
				if ((iDate == min) && WasMinus)
				{
					pstring oldX = x;
					x = "-";
					x += oldX;
				}
			}
			if (First && (iDate > 2) && (iDate == min)) c = ' ';
			else c = '0';
			First = false;
			while (x.length() < n) {
				pstring oldX = x;
				x = "";
				x.Append(c);
				x += oldX;
			}
			if (iDate < 3) x = copy(x, x.length() - n + 1, n);
			s = s + x;
		}
	}
label1:
	return s;
}

double AddMonth(double R, double RM)
{
	WORD d, m, y;
	SplitDate(R, d, m, y);
	const longint l = y * 12 + m - 1 + static_cast<longint>(trunc(RM));
	double intpart;
	const double RTime = modf(R, &intpart);
	y = static_cast<WORD>(l / 12);
	m = (l % 12) + 1;
	if (d > NoDayInMonth[m]) {
		d = NoDayInMonth[m];
		if (m == 2 && OlympYear(y)) d = 29;
	}
	return RDate(y, m, d, 0, 0, 0, 0) + RTime;
}

double DifMonth(double R1, double R2)
{
	WORD d1, m1, y1, d2, m2, y2;
	SplitDate(R1, d1, m1, y1);
	SplitDate(R2, d2, m2, y2);
	const int y = y2 - y1;
	const int m = m2 - m1;
	return y * 12 + m;
}

bool IsLetter(char C)
{
	if (C >= 'a' && C <= 'z') return true;
	if (C >= 'A' && C <= 'Z') return true;
	if (C == '_') return true;
	if (C < 0) return true; // ekviv. >= 0x80;
	return false;
}

void MyMove(void* A1, void* A2, WORD N)
{
	memcpy(A2, A1, N);
}

FILE* GetOverHandle(FILE* fptr, int diff)
{
	ptrdiff_t pos = find(vOverHandle.begin(), vOverHandle.end(), fptr) - vOverHandle.begin();
	int newPos = pos + diff;
	if (newPos >= 0 && newPos < vOverHandle.size() - 1) { return vOverHandle[pos - 1]; }
	return nullptr;
}

bool IsHandle(FILE* H)
{
	if (H == nullptr) return false;
	return Handles.count(H) > 0;
}

bool IsUpdHandle(FILE* H)
{
	if (H == nullptr) return false;
	return UpdHandles.count(H) > 0;
}

bool IsFlshHandle(FILE* H)
{
	if (H == nullptr) return false;
	return FlshHandles.count(H) > 0;
}

void SetHandle(FILE* H)
{
	if (H == nullptr) return;
	Handles.insert(H);
	//CardHandles++;
}

void SetUpdHandle(FILE* H)
{
	if (H == nullptr) return;
	UpdHandles.insert(H);
}

/// vrati pocet stejnych znaku na zacatku retezce
WORD SLeadEqu(pstring S1, pstring S2)
{
	WORD count = 0;
	// pocet znaku k otestovani
	WORD minLen = min(S1.length(), S2.length());
	for (size_t i = 0; i < minLen; i++) {
		if (S1[i + 1] == S2[i + 1]) {
			count++;
			continue;
		}
		break;
	}
	return count;
}

void SetFlshHandle(FILE* H)
{
	if (H == nullptr) return;
	FlshHandles.insert(H);
}

void ResetHandle(FILE* H)
{
	if (H == nullptr) return;
	Handles.erase(H);
	//CardHandles--;
}

void ResetUpdHandle(FILE* H)
{
	if (H == nullptr) return;
	UpdHandles.erase(H);
}

void ResetFlshHandle(FILE* H)
{
	if (H == nullptr) return;
	FlshHandles.erase(H);
}

void ClearHandles()
{
	Handles.clear();
	//CardHandles = 0;
}

void ClearUpdHandles()
{
	UpdHandles.clear();
}

void ClearFlshHandles()
{
	FlshHandles.clear();
}

bool IsNetCVol()
{
#ifdef FandNetV
	return CVol == "#" || CVol == "##" || EquUpCase(CVol, "#R");
#else
	return false;
#endif
}

void ExtendHandles()
{
	// pøesouvá OldHTPtr na NewHT
}

void UnExtendHandles()
{
	// zavøe všechny otevøené soubory, pøesune zpìt NewHT do Old... promìnných
}

FILE* OpenH(FileOpenMode Mode, FileUseMode UM)
{
	// $3C vytvoøí nebo pøepíše soubor
	// $3D otevírá exitující soubor
	// $5B vytvoøí nový soubor - pokud již exituje, vyhodí chybu
	//
	// bit 0: read-only, 1: hidden, 2: system, 3: volume label, 4: reserved, must be zero (directory)
	//        5: archive bit, 7: if set, file is shareable under Novell NetWare
	//
	// pøi 'IsNetCVol' se chová jinak
	// RdOnly $20, RdShared $40, Shared $42, Exclusive $12

	std::string txt[] = { "Clos", "OpRd", "OpRs", "OpSh", "OpEx" };

	//if (CardHandles == files) RunError(884);
	longint w = 0;
	std::string openFlags;
label1:
	switch (Mode) {
	case _isoldfile:
	case _isoldnewfile:
	{
		openFlags = UM == RdOnly ? "rb" : "r+b";
		break;
	}
	case _isoverwritefile:
	{
		openFlags = "w+b";
		break;
	}
	case _isnewfile:
	{
		openFlags = "w+b"; // UM == RdOnly ? "w+b" : "w+b";
		break;
	}
	}

	FILE* nFile = nullptr;
	HandleError = (WORD)fopen_s(&nFile, CPath.c_str(), openFlags.c_str());

	// https://docs.microsoft.com/en-us/cpp/c-runtime-library/errno-doserrno-sys-errlist-and-sys-nerr?view=vs-2019
	if (IsNetCVol() && (HandleError == EACCES || HandleError == ENOLCK))
	{
		if (w == 0)
		{
			SetMsgPar(CPath, txt[UM]);
			w = PushWrLLMsg(825, false);
		}
		LockBeep();
		KbdTimer(spec.NetDelay, 0);
		goto label1;
	}

	if (HandleError == 0)
	{
		SetHandle(nFile);
		if (Mode != _isoldfile) SetUpdHandle(nFile);
	}

	else if (HandleError == ENOENT) // No such file or directory
	{

		if (/*Mode == _isoldfile ||*/ Mode == _isoldnewfile)
		{
			Mode = _isnewfile;
			goto label1;
		}
	}
	if (w != 0) PopW(w);

	Logging* log = Logging::getInstance();
	log->log(loglevel::DEBUG, "opening file  0x%p '%s', error %i", nFile, CPath.c_str(), HandleError);

	// pridani FILE* do vektoru kvuli 'WORD OvrHandle = h - 1;'
	vOverHandle.push_back(nFile);
#ifdef _DEBUG
	if (filesMap.find(CPath) != filesMap.end()) {
		// soubor uz v mape je, budeme aktualizovat
		filesMap[CPath] = DataFile(CPath, CFile, nFile);
	}
	else {
		filesMap.insert(std::pair<std::string, DataFile>(CPath, DataFile(CPath, CFile, nFile)));
	}
#endif
	return nFile;
}

WORD ReadLongH(FILE* handle, longint bytes, void* buffer)
{
	if (handle == nullptr) RunError(706);
	if (bytes <= 0) return 0;
	auto readed = fread_s(buffer, bytes, 1, bytes, handle);
	if (readed != static_cast<unsigned int>(bytes))
	{
		// nebyl nacten pozadovany pocet B
		auto eofReached = feof(handle);
		HandleError = ferror(handle);
	}
	return WORD(readed);
}

void WriteLongH(FILE* handle, longint bytes, void* buffer)
{
	//if (CFile != nullptr && CFile->Name == "PARAM3")
	//{
	//	printf("");
	//}
	if (handle == nullptr) RunError(706);
	if (bytes <= 0) return;
	// uloží do souboru daný poèet Bytù z bufferu
	fwrite(buffer, 1, bytes, handle);
	HandleError = ferror(handle);
}

void WriteH(FILE* handle, WORD bytes, void* buffer)
{
	//if ((uintptr_t)handle == 0x0117d388)
	//{
	//	printf("");
	//}
	WriteLongH(handle, bytes, buffer);
}

longint FileSizeH(FILE* handle)
{
	longint pos = PosH(handle);
	auto result = MoveH(0, 2, handle);
	SeekH(handle, pos);
	return result;
}

longint SwapLong(longint N)
{
	return 0;
}

void TruncH(FILE* handle, longint N)
{
	// cilem je zkratit delku souboru na N
	if (handle == nullptr) return;
	if (FileSizeH(handle) > N) {
		SeekH(handle, N);
		SetEndOfFile(handle);
		SeekH(handle, 0);
		SetFileValidData(handle, N);
	}
}

void CloseH(FILE** handle)
{
	Logging* log = Logging::getInstance();
	DataFile* fileForClose = nullptr;
	if (*handle == nullptr) return;
	// uzavre soubor
	auto res = fclose(*handle);
	WORD HandleError = res;
	log->log(loglevel::DEBUG, "closing file 0x%p '%s', error %i",
		*handle, fileForClose == nullptr ? "nullptr" : fileForClose->Name.c_str(), res);

	if (res != 0) {
		throw std::exception("Cannot close file!");
	}

#ifdef _DEBUG
	// oznaci za uzavreny ve filesMap
	for (auto& f : filesMap)
	{
		if (f.second.Handler == *handle) {
			fileForClose = &f.second;
			f.second.SetClose();
			break;
		}
	}
	if (fileForClose == nullptr) {
		// soubor ve filesMap nebyl
		log->log(loglevel::WARN, "closing file 0x%p, but file wasn't in filesMap!", *handle);
	}
#endif

	* handle = nullptr;
}

void ClearCacheH(FILE* h)
{
	Logging* log = Logging::getInstance();
	log->log(loglevel::DEBUG, "ClearCacheH() 0x%p", h);
	// smazeme cache
	cache.SaveRemoveCache(h);
}

void CloseClearH(FILE** h)
{
	Logging* log = Logging::getInstance();
	log->log(loglevel::DEBUG, "CloseClearH() 0x%p", h);
	if (h == nullptr) return;
	ClearCacheH(*h);
	CloseH(h);
}

void SetFileAttr(WORD Attr)
{
	// nastaví atributy souboru/adresáøe
	// 0 = read only, 1 = hidden file, 2 = system file, 3 = volume label, 4 = subdirectory,
	// 5 = written since backup, 8 = shareable (Novell NetWare)
	if (SetFileAttributesA(CPath.c_str(), Attr) == 0)
	{
		HandleError = GetLastError();
	}
}

WORD GetFileAttr()
{
	// ziska atributy souboru / adresare
	auto result = GetFileAttributesA(CPath.c_str());
	if (result == INVALID_FILE_ATTRIBUTES) {
		HandleError = GetLastError();
		return 0;
	}
	else
	{
		HandleError = 0;
		return result;
	}
}

//CachePage* Cache(FILE* Handle, longint Page)
//{
//	return nullptr;
//}

void RdWrCache(bool ReadOp, FILE* Handle, bool NotCached, longint Pos, WORD N, void* Buf)
{
	Logging* log = Logging::getInstance();

	bool Cached = !NotCached;
	integer PgeIdx = 0, PgeRest = 0; WORD err = 0; longint PgeNo = 0;
	//CachePage* Z = nullptr;

	if (Handle == nullptr) {
		RunError(706);
		return;
	}

	if (!ReadOp && (CFile != nullptr) && (CFile->UMode == RdOnly)) {
		// snazime se zapsat do RdOnly souboru
		// zapisem pouze do cache
		// TODO: nutno doresit, co s tim dal ...
		log->log(loglevel::ERR, "RdWrCache() trying to write to RdOnly file 0x%p!", Handle);
		FileCache* c1 = cache.GetCache(Handle);
		c1->Save(Pos, N, (unsigned char*)Buf);
		return;
	}

	// writing to the file -> Set Update Flag
	if (!ReadOp) {
		SetUpdHandle(Handle);
	}

	if (Cached) {
		//log->log(loglevel::DEBUG, "RdWrCache() 0x%p cached file operation.", Handle);
		FileCache* c1 = cache.GetCache(Handle);
		if (ReadOp) {
			auto src = c1->Load(Pos);
			if (src == nullptr) return;
			memcpy(Buf, src, N);
		}
		else {
			c1->Save(Pos, N, (unsigned char*)Buf);
		}
	}
	else {
		// soubor nema cache, cteme (zapisujeme) primo z disku (na disk)
		//log->log(loglevel::DEBUG, "RdWrCache() non cached file 0x%p operation.", Handle);
		SeekH(Handle, Pos);
		if (ReadOp) ReadH(Handle, N, Buf);
		else WriteH(Handle, N, Buf);
		if (HandleError == 0) return;
		err = HandleError;
		SetCPathForH(Handle);
		SetMsgPar(CPath);
		RunError(700 + err);
	}
}

void FlushH(FILE* handle)
{
	Logging* log = Logging::getInstance();
	log->log(loglevel::DEBUG, "FlushH()      0x%p", handle);
	if (handle == nullptr) return;
	auto result = fflush(handle);
	if (result == EOF) { HandleError = result; }
	//SetHandle(handle);
	SetUpdHandle(handle);
	//CloseH(handle);
}

void DeleteFile(pstring path)
{
}

WORD FindCtrlM(LongStr* s, WORD i, WORD n)
{
	WORD l = s->LL;
	while (i <= l - 1) {
		if (s->A[i] == '\r') {
			if (n > 1) n--;
			else return i;
		}
		i++;
	}
	return l + 1;
}

WORD FindCtrlM(std::string& s, WORD i, WORD n)
{
	size_t l = s.length();
	while (i <= l - 1) {
		if (s[i] == '\r') {
			if (n > 1) n--;
			else return i;
		}
		i++;
	}
	return l + 1;
}

WORD SkipCtrlMJ(LongStr* s, WORD i)
{
	WORD l = s->LL;
	if (i <= l - 1) {
		i++;
		if (i <= l - 1 && s->A[i] == '\n') i++;
	}
	return i;
}

WORD SkipCtrlMJ(std::string& s, WORD i)
{
	size_t l = s.length();
	if (i <= l - 1) {
		i++;
		if (i <= l - 1 && s[i] == '\n') i++;
	}
	return i;
}

void FlushHandles()
{
	for (auto handle : UpdHandles)
	{
		FlushH(handle);
	}
	for (auto handle : FlshHandles)
	{
		FlushH(handle);
	}
	ClearUpdHandles();
	ClearFlshHandles();
}

longint GetDateTimeH(FILE* handle)
{
	if (handle == nullptr) return -1;
	// vrátí èas posledního zápisu souboru + datum posledního zápisu souboru
	// 2 + 2 Byte (datum vlevo, èas vpravo)
	FILETIME ft;
	auto result = GetFileTime(handle, nullptr, nullptr, &ft);
	if (result == 0) HandleError = GetLastError();
	return (ft.dwHighDateTime << 16) + ft.dwLowDateTime;
}

void MyDeleteFile(pstring path)
{
	// smaže soubor - INT $41
	auto result = remove(path.c_str());
	if (result != 0) HandleError = result;
}

void RenameFile56(pstring OldPath, pstring NewPath, bool Msg)
{
	// presouva nebo prejmenovava soubor
	// potom:
	auto result = rename(OldPath.c_str(), NewPath.c_str());
	if (result != 0) HandleError = result;
	if (Msg && HandleError != 0)
	{
		SetMsgPar(OldPath, NewPath);
		RunError(829);
	}
}

std::string MyFExpand(std::string Nm, std::string EnvName)
{
	std::string d = GetDir(0);
	std::string f = FandDir;
	DelBackSlash(f);
	//ChDir(f);
	std::string p = GetEnv(EnvName.c_str());
	AddBackSlash(p);
	if (!p.empty()) p += Nm;
	else {
		std::string envp = GetEnv("PATH");
		p = FSearch(Nm, f + ";" + envp);
		if (p.empty()) p = Nm;
	}
	std::string result = FExpand(p);
	//ChDir(d);
	return p;
}

void* Normalize(longint L)
{
	return nullptr;
}

longint AbsAdr(void* P)
{
	return 0;
}

WORD LogToAbsLenStyleStr(pstring s, WORD l)
{
	WORD i = 1;
	while ((i <= s.length()) && (l > 0))
	{
		if (!(s[i] == 0x13 || s[i] == 0x17 || s[i] == 0x11 || s[i] == 0x04
			|| s[i] == 0x02 || s[i] == 0x05 || s[i] == 0x01)) l--;
		i++;
	}
	return i - 1;
}

bool CacheExist()
{
	return true;
	//return !cache.Empty();
}

void SetMyHeapEnd()
{
	// MyHeapEnd = ptr(PtrRec(CacheEnd).Seg - CachePageSz, PtrRec(CacheEnd).Ofs);
}

bool WrCPage(FILE* Handle, longint N, void* Buf, WORD ErrH)
{
	return true;
}

void LockCache()
{
}

void UnLockCache()
{
}

bool SaveCache(WORD ErrH, FILE* f)
{
	// ulozi cache do souboru
	cache.SaveRemoveCache(f);
	return true;
}

void SubstHandle(WORD h1, WORD h2)
{
}

integer HeapErrFun(WORD Size)
{
	return 0;
}

void AlignParagraph()
{
}

void* GetStore2(WORD Size)
{
	return nullptr;
}

void* GetZStore2(WORD Size)
{
	return nullptr;
}

std::string* StoreStr(std::string S)
{
	auto nss = new std::string(S);
	return nss;
}

void MarkStore(void* p)
{
}

void MarkStore2(void* p)
{
}

void MarkBoth(void* p, void* p2)
{
}

void ReleaseStore(void* pointer)
{
	delete[] pointer;
}

void ReleaseAfterLongStr(void* pointer)
{
	delete[] pointer;
}

bool OverlapByteStr(void* p1, void* p2)
{
	return false;
}

bool MouseInRectProc(WORD X, WORD Y, WORD XSize, WORD Size)
{
	return false;
}

bool EqualsMask(void* p, WORD l, pstring Mask)
{
	if (Mask.length() < 1) return false;
	std::string Value = std::string((char*)p, l);
	return CmpStringWithMask(Value, Mask);

	// puvodni implementace pred pouzitim regexu
	//if (Mask.length() < l) return false;
	//BYTE* inp = (BYTE*)p;
	//for (size_t i = 0; i < l; i++)
	//{
	//	if (inp[i] != Mask[i + 1]) return false;
	//}
	//return true;
}

bool EquLongStr(LongStr* S1, LongStr* S2)
{
	if (S1->LL != S2->LL) return false;
	if (S1->LL == 0) return true;
	for (size_t i = 0; i < S1->LL; i++)
	{
		if (S1->A[i] != S2->A[i]) return false;
	}
	return true;
}

bool EquArea(void* P1, void* P2, WORD L)
{
	auto* c1 = static_cast<unsigned char*>(P1);
	auto* c2 = static_cast<unsigned char*>(P2);
	for (size_t i = 0; i < L; i++)
	{
		if (c1[i] != c2[i]) return false;
	}
	return true;
}

WORD ListLength(void* P)
{
	return 0;
}

void ReleaseStore2(void* p)
{
}

void ReleaseBoth(void* p, void* p2)
{
	ReleaseStore(p);
	ReleaseStore2(p2);
}

int StoreAvail()
{
	return 512 * 1024;
}

void AlignLongStr()
{
}

void NewExit(PProcedure POvr, ExitRecord* Buf)
{
}

void GoExit()
{
	Logging* log = Logging::getInstance();
	log->log(loglevel::WARN, "GoExit(): '%s'", MsgLine.c_str());
#ifdef _DEBUG
	screen.ScrWrText(1, 1, MsgLine.c_str());
#endif
	BreakP = true;
	log->log(loglevel::WARN, "GoExit(): Setting 'BreakP = true'", MsgLine.c_str());
}

void RestoreExit(ExitRecord& Buf)
{
}

bool OSshell(std::string Path, std::string CmdLine, bool NoCancel, bool FreeMm, bool LdFont, bool TextMd)
{
	Logging* log = Logging::getInstance();

	char psBuffer[128];
	FILE* pPipe;

	std::string cmd = Path.empty() ? CmdLine : Path + " " + CmdLine;
	log->log(loglevel::INFO, "OSshell() calling command '%s'", cmd.c_str());

	if ((pPipe = _popen(cmd.c_str(), "rt")) == nullptr)
		return false;


	while (fgets(psBuffer, 128, pPipe)) {
		puts(psBuffer);
	}

	if (feof(pPipe)) {
		LastExitCode = _pclose(pPipe);
	}
	else {
		LastExitCode = -1;
	}

	return true;
}

std::string PrTab(WORD printerNr, WORD value)
{
	std::string result;
	if (printer[printerNr].Strg.empty()) result = "";

	size_t offset = 0;
	for (size_t i = 0; i < value; i++) {
		offset += printer[printerNr].Strg[offset++];
	}

	BYTE length = printer[printerNr].Strg[offset];
	result = printer[printerNr].Strg.substr(offset + 1, length);
	return result;
}

void SetCurrPrinter(integer NewPr)
{
	if (NewPr >= prMax) return;
	if (prCurr >= 0) {/* !!! with printer[prCurr] do!!! */
		if (printer[prCurr].TmOut != 0) {
			PrTimeOut[printer[prCurr].Lpti] = OldPrTimeOut[printer[prCurr].Lpti];
		}
	}
	prCurr = NewPr;
	if (prCurr >= 0) { /* !!! with printer[prCurr] do!!! */
		if (printer[prCurr].TmOut != 0) {
			PrTimeOut[printer[prCurr].Lpti] = printer[prCurr].TmOut;
		}
	}
}

void (*ExitSave)(); //535

void WrTurboErr()
{
	pstring s = pstring(9);
	str(ExitCode, s);
	SetMsgPar(s);
	WrLLF10Msg(626);
	ErrorAddr = nullptr;
	ExitCode = 0;
}

void MyExit()
{
	// { asm mov ax, SEG @Data; mov ds, ax end; }
	ExitProc = ExitSave;
	if (!WasInitPgm) { UnExtendHandles(); goto label1; }

	if (ErrorAddr != nullptr)
		switch (ExitCode)
		{
		case 202: // {stack overflow}
		{
			// asm mov sp, ExitBuf.rSP
			WrLLF10Msg(625);
			break;
		}
		case 209: //{overlay read error}
			WrLLF10Msg(648);
			break;
		default: WrTurboErr(); break;
		}
#ifdef FandSQL
	SQLDisconnect();
#endif

	UnExtendHandles();
	MyDeleteFile(FandWorkName);
	MyDeleteFile(FandWorkXName);
	MyDeleteFile(FandWorkTName);
	// TODO? CloseXMS();
label1:
	if (WasInitDrivers) {
		// TODO? DoneMouseEvents();
		// CrsIntrDone();
		BreakIntrDone();
		if (IsGraphMode) {
			CloseGraph();
			IsGraphMode = false;
			// TODO? ScrSeg = video.Address;
			/*asm  push bp; mov ah,0fH; int 10H; cmp al,StartMode; je @1;
				 mov ah,0; mov al,StartMode; int 10H;
			@1:  pop bp end; */
			screen.Window(1, 1, TxtCols, TxtRows);
			TextAttr = StartAttr;
			ClrScr();
			screen.CrsNorm();
			ChDir(OldDir);
			SetCurrPrinter(-1);
		}
		if (ExitCode == 202) Halt(202);
	}
}

void OpenResFile()
{
	CPath = FandResName;
	CVol = "";
	ResFile.Handle = OpenH(_isoldfile, RdOnly);
	ResFile.FullName = CPath;
	if (HandleError != 0)
	{
		printf("can't open %s", FandResName.c_str());
		wait();
		Halt(0);
	}
}

void OpenWorkH()
{
	CPath = FandWorkName;
	CVol = "";
	WorkHandle = OpenH(_isoldnewfile, Exclusive);
	if (HandleError != 0) {
		printf("can't open %s", FandWorkName.c_str());
		wait();
		Halt(-1);
	}
}

void NonameStartFunction()
{
	// TODO:
	// CurPSP = ptr(PrefixSeg, 0);
	// MyHeapEnd = HeapEnd;
	ExtendHandles();
	prCurr = -1;
	ExitSave = ExitProc;
	ExitProc = MyExit;
	MyBP = nullptr;
	// integer UserLicNr = WORD(UserLicNrShow) & 0x7FFF;
	FandResName = MyFExpand("FAND.RES", "FANDRES");
	OpenResFile();
}
