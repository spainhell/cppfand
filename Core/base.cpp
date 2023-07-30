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
#include <iostream>

#include "Cfg.h"
#include "compile.h"
#include "FileD.h"
#include "GlobalVariables.h"
#include "../datafiles/datafiles.h"
#include "../Common/exprcmp.h"
#include "../Common/compare.h"
#include "../Common/codePages.h"
#include "../fandio/cache.h"
#include "../fandio/files.h"
#include "../Logging/Logging.h"


Video video;
Spec spec;
Fonts fonts;
Colors colors;

WORD TxtCols = 80;
WORD TxtRows = 25;

short prCurr, prMax;

wdaystt WDaysTabType;
WORD NWDaysTab;
double WDaysFirst;
double WDaysLast;
wdaystt* WDaysTab;

char AbbrYes = 'Y';
char AbbrNo = 'N';

unsigned long HandleError;
pstring OldDir;
pstring FandDir;
std::string WrkDir;
std::string FandResName;
pstring FandWorkName;
pstring FandWorkXName;
pstring FandWorkTName;
std::string CPath;
std::string CDir;
std::string CName;
std::string CExt;
std::string CVol;

ResFile resFile;

WORD F10SpecKey; // r. 293
BYTE ProcAttr;
// bool SetStyleAttr(char c, BYTE& a); // je v KBDWW
std::string MsgLine;
std::string MsgPar[4];

WORD OldNumH; // r1 
void* OldHTPtr = nullptr;

Cache cache;
//std::map<FILE*, FileCache*> Cache::cacheMap;
WORD CachePageSize;
void* AfterCatFD; // r108
WORD BPBound; // r212
bool ExitP, BreakP;
int LastExitCode = 0; // r215
bool WasLPTCancel;
HANDLE WorkHandle;
int MaxWSize = 0; // {currently occupied in FANDWORK.$$$}
Printer printer[10];
TPrTimeOut OldPrTimeOut;
TPrTimeOut PrTimeOut;  // absolute 0:$478;
bool WasInitDrivers = false;
bool WasInitPgm = false;
WORD LANNode; // r. 431
void (*CallOpenFandFiles)(); // r453
void (*CallCloseFandFiles)(); // r454

double userToday = 0;
__int32 UserLicNr = 0;

typedef FILE* filePtr;

std::set<HANDLE> Handles;
std::set<HANDLE> UpdHandles;
std::set<HANDLE> FlshHandles;

//map<WORD, FILE*> fileMap;
// nahrada za 'WORD OvrHandle = h - 1' - zjisteni predchoziho otevreneho souboru;
std::vector<HANDLE> vOverHandle;

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

//long PosH(FILE* handle)
//{
//	if (handle == nullptr) return -1;
//	try
//	{
//		const long result = ftell(handle);
//		HandleError = ferror(handle);
//		return static_cast<int>(result);
//	}
//	catch (const std::exception& e)
//	{
//		std::cout << e.what() << "\n";
//		return -1;
//	}
//}

long MoveH(long offset, int origin, FILE* handle)
{
	if (handle == nullptr) return -1;
	// offset - hodnota offsetu
	// origin: 0 - od zacatku, 1 - od aktualni, 2 - od konce
	// handle - file handle
	try
	{
		auto result = fseek(handle, offset, origin);
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

long SeekH(HANDLE handle, size_t offset)
{
	if (handle == nullptr) RunError(705);
	return SeekF(handle, HandleError, offset, 0);
}

size_t ReadH(HANDLE handle, size_t length, void* buffer)
{
	return ReadF(handle, buffer, length, HandleError);
}

std::string ReadMessage(int N)
{
	std::string s;
	bool result = resFile.ReadMessage(N, s);
	if (!result) {
		MsgPar[0] = std::to_string(N);
	}

	ConvKamenToCurr(s, !fonts.NoDiakrSupported);

	std::string message;
	size_t param_index = 0;
	for (size_t i = 0; i < s.length(); i++) {
		if (s[i] == '$' && s[i + 1] != '$') {
			message += MsgPar[param_index++];
		}
		else {
			message += s[i];
			if (s[i] == '$') i++;
		}
	}

	MsgLine = message;
	return message;
}

void WriteMsg(WORD N)
{
}

void ClearLL(BYTE attr)
{
	screen.ScrClr(1, TxtRows, TxtCols, 1, ' ', colors.uNorm);
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

void StrLPCopy(char* Dest, pstring s, WORD MaxL)
{
	auto sLen = s.length();
	int len = (sLen < MaxL) ? sLen : MaxL;
	Move((void*)s.c_str(), Dest, len);
}

short MinI(short X, short Y)
{
	if (X < Y) return X;
	return Y;
}

short MaxI(short X, short Y)
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

int MinL(int X, int Y)
{
	if (X < Y) return X;
	return Y;
}

int MaxL(int X, int Y)
{
	if (X > Y) return X;
	return Y;
}



void wait()
{
}

bool MouseInRect(WORD X, WORD Y, WORD XSize, WORD Size)
{
	return false;
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

HANDLE GetOverHandle(HANDLE fptr, int diff)
{
	ptrdiff_t pos = find(vOverHandle.begin(), vOverHandle.end(), fptr) - vOverHandle.begin();
	int newPos = pos + diff;
	if (newPos >= 0 && newPos < vOverHandle.size() - 1) { return vOverHandle[pos - 1]; }
	return nullptr;
}

bool IsHandle(HANDLE H)
{
	if (H == nullptr) return false;
	return Handles.count(H) > 0;
}

bool IsUpdHandle(HANDLE H)
{
	if (H == nullptr) return false;
	return UpdHandles.count(H) > 0;
}

bool IsFlshHandle(HANDLE H)
{
	if (H == nullptr) return false;
	return FlshHandles.count(H) > 0;
}

void SetHandle(HANDLE H)
{
	if (H == nullptr) return;
	Handles.insert(H);
	//CardHandles++;
}

void SetUpdHandle(HANDLE H)
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

WORD SLeadEqu(const std::string& s1, const std::string& s2)
{
	WORD count = 0;
	// pocet znaku k otestovani
	WORD minLen = min(s1.length(), s2.length());
	for (size_t i = 0; i < minLen; i++) {
		if (s1[i] == s2[i]) {
			count++;
			continue;
		}
		break;
	}
	return count;
}

void SetFlshHandle(HANDLE H)
{
	if (H == nullptr) return;
	FlshHandles.insert(H);
}

void ResetHandle(HANDLE H)
{
	if (H == nullptr) return;
	Handles.erase(H);
	//CardHandles--;
}

void ResetUpdHandle(HANDLE H)
{
	if (H == nullptr) return;
	UpdHandles.erase(H);
}

void ResetFlshHandle(HANDLE H)
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
	// presouva OldHTPtr na NewHT
}

void UnExtendHandles()
{
	// zavre vsechny otevrene soubory, presune zpet NewHT do Old... promennych
}

HANDLE OpenH(const std::string& path, FileOpenMode Mode, FileUseMode UM)
{
	// $3C vytvori nebo prepise soubor
	// $3D otevira exitujici soubor
	// $5B vytvori novy soubor - pokud jiz exituje, vyhodi chybu
	//
	// bit 0: read-only, 1: hidden, 2: system, 3: volume label, 4: reserved, must be zero (directory)
	//     5: archive bit, 7: if set, file is shareable under Novell NetWare
	//
	// pri 'IsNetCVol' se chova jinak
	// RdOnly $20, RdShared $40, Shared $42, Exclusive $12

	std::string txt[] = { "Close", "OpRd", "OpRs", "OpSh", "OpEx" };

	//if (CardHandles == files) RunError(884);
	int w = 0;

	HANDLE handle;
	DWORD access_mode = 0;
	DWORD share_mode = 0;
	DWORD create_mode = 0;
	
	switch (Mode) {
	case _isOldFile: {
		create_mode = OPEN_EXISTING;
		break;
		}
	case _isOldNewFile: {
		create_mode = OPEN_EXISTING;
		break;
	}
	case _isOverwriteFile: {
		create_mode = CREATE_ALWAYS;
		break;
	}
	case _isNewFile: {
		create_mode = CREATE_NEW;
		break;
	}
	}

	switch (UM) {
	case RdOnly: {
		access_mode = GENERIC_READ;
		share_mode = FILE_SHARE_READ;
		break;
	}
	case RdShared: {
		access_mode = GENERIC_READ;
		share_mode = FILE_SHARE_READ;
		break;
	}
	case Shared: {
		access_mode = GENERIC_READ | GENERIC_WRITE;
		share_mode = FILE_SHARE_READ | FILE_SHARE_WRITE;
		break;
	}
	case Exclusive: {
		access_mode = GENERIC_READ | GENERIC_WRITE;
		share_mode = 0;
		break;
	}
	default: ;
	}

	while (true) {
		//HandleError = (WORD)fopen_s(&nFile, path.c_str(), openFlags.c_str());
		handle = OpenF(path, HandleError, access_mode, share_mode, create_mode, 128);
		if (handle == INVALID_HANDLE_VALUE) {
			handle = nullptr;
		}

		// https://docs.microsoft.com/en-us/cpp/c-runtime-library/errno-doserrno-sys-errlist-and-sys-nerr?view=vs-2019
		if (IsNetCVol() && (HandleError == EACCES || HandleError == ENOLCK)) {
			if (w == 0) {
				SetMsgPar(path, txt[UM]);
				w = PushWrLLMsg(825, false);
			}
			LockBeep();
			KbdTimer(spec.NetDelay, 0);
			continue;
		}

		if (HandleError == 0)
		{
			SetHandle(handle);
			if (Mode != _isOldFile) SetUpdHandle(handle);
		}

		else if (HandleError == ENOENT) {
			// No such file or directory
			if (/*Mode == _isOldFile ||*/ Mode == _isOldNewFile) {
				Mode = _isNewFile;
				create_mode = CREATE_NEW;
				continue;
			}
		}
		if (w != 0) PopW(w);
		break;
	}

	Logging* log = Logging::getInstance();
	log->log(loglevel::DEBUG, "opening file  0x%p '%s', error %i", handle, path.c_str(), HandleError);

	// pridani FILE* do vektoru kvuli 'WORD OvrHandle = h - 1;'
	vOverHandle.push_back((FILE*)handle);

#ifdef _DEBUG
	if (filesMap.find(path) != filesMap.end()) {
		// soubor uz v mape je, budeme aktualizovat
		filesMap[path] = DataFile(path, CFile, (FILE*)handle);
		if (CFile != nullptr) {
			CFile->FullPath = CPath;
		}
	}
	else {
		filesMap.insert(std::pair(path, DataFile(path, CFile, (FILE*)handle)));
	}
#endif

	return (FILE*)handle;
}

WORD ReadLongH(FILE* handle, int bytes, void* buffer)
{
	if (handle == nullptr) RunError(706);
	if (bytes <= 0) return 0;
	auto readed = fread_s(buffer, bytes, 1, bytes, handle);
	if (readed != static_cast<unsigned int>(bytes)) {
		// nebyl nacten pozadovany pocet B
		auto eofReached = feof(handle);
		HandleError = ferror(handle);
	}
	return WORD(readed);
}

void WriteH(HANDLE handle, size_t length, void* buffer)
{
	if (handle == nullptr) RunError(706);
	if (length <= 0) return;
	//fwrite(buffer, 1, length, handle);
	//HandleError = ferror(handle);
	WriteF(handle, buffer, length, HandleError);
}

long FileSizeH(HANDLE handle)
{
	//int pos = PosH(handle);
	//long result = MoveH(0, 2, handle);
	//SeekH(handle, pos);
	long size = SizeF(handle, HandleError);
	return size;
}

//void TruncH(FILE* handle, size_t N)
//{
//	// cilem je zkratit delku souboru na N
//	if (handle == nullptr) return;
//	if (FileSizeH(handle) > N) {
//		//_chsize((int)handle, N);
//		SeekH(handle, N);
//		int result = SetEndOfFile(handle);
//		//SeekH(handle, 0);
//		//SetFileValidData(handle, N);
//		DWORD error = GetLastError();
//		printf("%i", result);
//	}
//}

void CloseH(HANDLE* handle)
{
	Logging* log = Logging::getInstance();
	DataFile* fileForClose = nullptr;
	if (handle == nullptr) return;
	// uzavre soubor
	bool res = CloseF(*handle, HandleError);
	log->log(loglevel::DEBUG, "closing file 0x%p '%s', error %i",
		*handle, fileForClose == nullptr ? "nullptr" : fileForClose->Name.c_str(), res);

	if (!res) {
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

	*handle = nullptr;
}

void ClearCacheH(HANDLE h)
{
	Logging* log = Logging::getInstance();
	//log->log(loglevel::DEBUG, "ClearCacheH() 0x%p", h);
	// smazeme cache
	cache.SaveRemoveCache(h);
}

void CloseClearH(HANDLE* h)
{
	Logging* log = Logging::getInstance();
	//log->log(loglevel::DEBUG, "CloseClearH() 0x%p", h);
	if (h == nullptr) return;
	ClearCacheH(*h);
	CloseH(h);
}

void RdWrCache(FileOperation operation, HANDLE handle, bool not_cached, size_t position, size_t count, void* buf)
{
	Logging* log = Logging::getInstance();

	bool Cached = !not_cached;
	short PgeIdx = 0, PgeRest = 0;
	WORD err = 0; int PgeNo = 0;
	//CachePage* Z = nullptr;

	if (handle == nullptr) {
		RunError(706);
		return;
	}

	if (operation == WRITE && (CFile != nullptr) && (CFile->FF->UMode == RdOnly)) {
		// snazime se zapsat do RdOnly souboru
		// zapisem pouze do cache
		// TODO: nutno doresit, co s tim dal ...
		log->log(loglevel::ERR, "RdWrCache() trying to write to RdOnly file 0x%p!", handle);
		FileCache* c1 = cache.GetCache(handle);
		c1->Save(position, count, (unsigned char*)buf);
		return;
	}

	// writing to the file -> Set Update Flag
	if (operation == WRITE) {
		SetUpdHandle(handle);
	}

	if (Cached) {
		//log->log(loglevel::DEBUG, "RdWrCache() 0x%p cached file operation.", handle);
		FileCache* c1 = cache.GetCache(handle);
		if (operation == READ) {
			auto src = c1->Load(position);
			if (src == nullptr) return;
			memcpy(buf, src, count);
		}
		else {
			c1->Save(position, count, (unsigned char*)buf);
		}
	}
	else {
		// soubor nema cache, cteme (zapisujeme) primo z disku (na disk)
		//log->log(loglevel::DEBUG, "RdWrCache() non cached file 0x%p operation.", handle);
		SeekH(handle, position);
		if (operation == READ) ReadH(handle, count, buf);
		else WriteH(handle, count, buf);
		if (HandleError == 0) return;
		err = HandleError;
		SetPathForH(handle);
		SetMsgPar(CPath);
		RunError(700 + err);
	}
}

//void FlushH(FILE* handle)
//{
//	Logging* log = Logging::getInstance();
//	//log->log(loglevel::DEBUG, "FlushH()      0x%p", handle);
//	if (handle == nullptr) return;
//	auto result = fflush(handle);
//	if (result == EOF) { HandleError = result; }
//	//SetHandle(handle);
//	SetUpdHandle(handle);
//	//CloseH(handle);
//}

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
	for (auto handle : UpdHandles)	{
		FlushF(handle, HandleError);
	}
	for (auto handle : FlshHandles) {
		FlushF(handle, HandleError);
	}
	ClearUpdHandles();
	ClearFlshHandles();
}

int GetDateTimeH(FILE* handle)
{
	if (handle == nullptr) return -1;
	// vrati cas posledniho zapisu souboru + datum posledniho zapisu souboru
	// 2 + 2 Byte (datum vlevo, cas vpravo)
	FILETIME ft;
	bool result = GetFileTime(handle, nullptr, nullptr, &ft);
	if (result == 0) HandleError = GetLastError();
	return (ft.dwHighDateTime << 16) + ft.dwLowDateTime;
}

void MyDeleteFile(pstring path)
{
	// smaze soubor - INT $41
	auto result = remove(path.c_str());
	if (result != 0) HandleError = result;
}

void RenameFile56(pstring OldPath, pstring NewPath, bool Msg)
{
	// presouva nebo prejmenovava soubor
	// potom:
	auto result = rename(OldPath.c_str(), NewPath.c_str());
	if (result != 0) HandleError = result;
	if (Msg && HandleError != 0) {
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
	return result;
}

WORD LogToAbsLenStyleStr(pstring s, WORD l)
{
	WORD i = 1;
	while ((i <= s.length()) && (l > 0)) {
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

bool WrCPage(FILE* Handle, int N, void* Buf, WORD ErrH)
{
	return true;
}

void LockCache()
{
	// implementation is empty in the file MEMORY.PAS too
}

void UnLockCache()
{
	// implementation is empty in the file MEMORY.PAS too
}

bool SaveCache(WORD ErrH, HANDLE f)
{
	// saves cache to the file
	cache.SaveRemoveCache(f);
	return true;
}

short HeapErrFun(WORD Size)
{
	return 0;
}

std::string* StoreStr(std::string S)
{
	auto nss = new std::string(S);
	return nss;
}

void MarkStore(void* p)
{
}

void MarkBoth(void* p, void* p2)
{
}

void ReleaseStore(void** pointer)
{
	delete[] *pointer; *pointer = nullptr;
}

void ReleaseAfterLongStr(void** pointer)
{
	delete[] *pointer; *pointer = nullptr;
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

bool EqualsMask(const std::string& value, std::string& mask)
{
	if (mask.length() < 1) return false;
	return CmpStringWithMask(value, mask);
}

bool EquLongStr(LongStr* S1, LongStr* S2)
{
	if (S1->LL != S2->LL) return false;
	if (S1->LL == 0) return true;
	for (size_t i = 0; i < S1->LL; i++)	{
		if (S1->A[i] != S2->A[i]) return false;
	}
	return true;
}

bool EquArea(void* P1, void* P2, WORD L)
{
	auto* c1 = static_cast<unsigned char*>(P1);
	auto* c2 = static_cast<unsigned char*>(P2);
	for (size_t i = 0; i < L; i++) {
		if (c1[i] != c2[i]) return false;
	}
	return true;
}

WORD ListLength(void* P)
{
	return 0;
}

int StoreAvail()
{
	return 512 * 1024;
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

bool OSshell(std::string path, std::string cmd_line, bool no_cancel, bool free_memory, bool load_font, bool text_mode)
{
	Logging* log = Logging::getInstance();

	char psBuffer[128];
	FILE* pPipe;

	std::string cmd = path.empty() ? cmd_line : path + " " + cmd_line;
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

void SetCurrPrinter(short NewPr)
{
	if (NewPr >= prMax) return;
	if (prCurr >= 0) {
		if (printer[prCurr].TmOut != 0) {
			PrTimeOut[printer[prCurr].Lpti] = OldPrTimeOut[printer[prCurr].Lpti];
		}
	}
	prCurr = NewPr;
	if (prCurr >= 0) {
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
	//MyDeleteFile(FandWorkXName);
	MyDeleteFile(FandWorkTName);
	// TODO? CloseXMS();
label1:
	if (WasInitDrivers) {
		// TODO? DoneMouseEvents();
		// CrsIntrDone();
		if (IsGraphMode) {
			CloseGraph();
			IsGraphMode = false;
			// TODO? ScrSeg = video.Address;
			/*asm  push bp; mov ah,0fH; int 10H; cmp al,StartMode; je @1;
				 mov ah,0; mov al,StartMode; int 10H;
			@1:  pop bp end; */
			screen.Window(1, 1, TxtCols, TxtRows);
			TextAttr = StartAttr;
			ClrScr(TextAttr);
			screen.CrsNorm();
			ChDir(OldDir);
			SetCurrPrinter(-1);
		}
		if (ExitCode == 202) Halt(202);
	}
}

void OpenWorkH()
{
	CPath = FandWorkName;
	CVol = "";
	WorkHandle = OpenH(CPath, _isOldNewFile, Exclusive);
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
	//MyBP = nullptr;
	UserLicNr = WORD(UserLicNrShow) & 0x7FFF;
	FandResName = MyFExpand("FAND.RES", "FANDRES");
	resFile.Open(FandResName);
}
