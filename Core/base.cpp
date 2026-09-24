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
#include <regex>

#include "Cfg.h"
#include "Compiler.h"
#include "GlobalVariables.h"
#include "../Common/exprcmp.h"
#include "../Common/compare.h"
#include "../Common/codePages.h"
#include "../Common/CommonVariables.h"
#include "../Logging/Logging.h"
#include "../fandio/directory.h"


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

std::string OldDir;
std::string FandDir;
std::string WrkDir;
std::string FandResName;
std::string FandWorkName;
std::string FandWorkXName;
std::string FandWorkTName;

ResFile resFile;

WORD F10SpecKey; // r. 293
uint8_t ProcAttr;
// bool SetStyleAttr(char c, uint8_t& a); // je v KBDWW
std::string MsgLine;
std::string MsgPar[4];

WORD OldNumH; // r1 
//void* OldHTPtr = nullptr;

//Cache cache;
//std::map<FILE*, FileCache*> Cache::cacheMap;
void* AfterCatFD; // r108
//WORD BPBound; // r212
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
void (*CallOpenFandFiles)(); // r453
void (*CallCloseFandFiles)(); // r454

double userToday = 0;
int32_t UserLicNr = 0;

typedef FILE* filePtr;

//std::set<HANDLE> Handles;
//std::set<HANDLE> UpdHandles;
//std::set<HANDLE> FlshHandles;

//map<WORD, FILE*> fileMap;
// nahrada za 'WORD OvrHandle = h - 1' - zjisteni predchoziho otevreneho souboru;
std::vector<HANDLE> vOverHandle;

void SetMsgPar(const std::string& s)
{
	MsgPar[0] = s;
}

void SetMsgPar(const std::string& s1, const std::string& s2)
{
	MsgPar[0] = s1;
	MsgPar[1] = s2;
}

void SetMsgPar(const std::string& s1, const std::string& s2, const std::string& s3)
{
	MsgPar[0] = s1;
	MsgPar[1] = s2;
	MsgPar[2] = s3;
}

void SetMsgPar(const std::string& s1, const std::string& s2, const std::string& s3, const std::string& s4)
{
	MsgPar[0] = s1;
	MsgPar[1] = s2;
	MsgPar[2] = s3;
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
	std::string s = ReadMessage(N);
	// TODO: fix ScrWrStr color
	screen.ScrWrStr(s, 0x07);

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

/// Je mys v obdelniku? Souradnice jsou znakove a 0-based, stejne jako Event.Where.
/// COMMON.PAS r175
bool MouseInRect(WORD X, WORD Y, WORD XSize, WORD Size)
{
	if (Event.Where.X < X || Event.Where.X >= X + XSize) return false;
	if (Event.Where.Y < Y || Event.Where.Y >= Y + Size) return false;
	return true;
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

WORD SkipCtrlMJ(std::string& s, WORD i)
{
	size_t l = s.length();
	if (i <= l - 1) {
		i++;
		if (i <= l - 1 && s[i] == '\n') i++;
	}
	return i;
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

void MarkStore(void* p)
{
}

void MarkBoth(void* p, void* p2)
{
}

void ReleaseStore(uint8_t** pointer)
{
	delete[] * pointer; *pointer = nullptr;
}

void ReleaseAfterLongStr(void** pointer)
{
	delete[] * pointer; *pointer = nullptr;
}

/// Varianta pro funkci MOUSEIN jazyka FANDu: v textovem rezimu jsou souradnice
/// 1-based, v grafickem uz v pixelech. COMMON.PAS r182
bool MouseInRectProc(WORD X, WORD Y, WORD XSize, WORD Size)
{
	if (IsGraphMode) {
		if (Event.WhereG.X < X || Event.WhereG.X >= X + XSize) return false;
		if (Event.WhereG.Y < Y || Event.WhereG.Y >= Y + Size) return false;
		return true;
	}
	const WORD x = X > 0 ? X - 1 : 0;
	const WORD y = Y > 0 ? Y - 1 : 0;
	if (Event.Where.X < x || Event.Where.X >= x + XSize) return false;
	if (Event.Where.Y < y || Event.Where.Y >= y + Size) return false;
	return true;
}

bool EqualsMask(void* p, WORD l, pstring Mask)
{
	if (Mask.length() < 1) return false;
	std::string Value = std::string((char*)p, l);
	return CmpStringWithMask(Value, Mask);
}

bool EqualsMask(const std::string& value, std::string& mask)
{
	if (mask.length() < 1) return false;
	return CmpStringWithMask(value, mask);
}

bool EquArea(void* p1, void* p2, size_t len)
{
	return memcmp(p1, p2, len) == 0;
}

int MemoryAvailable()
{
	return 512 * 1024;
}

[[noreturn]] void GoExit(const std::string& message)
{
	SPDLOG_WARN("GoExit(): '{}'", message);
#ifdef _DEBUG
	screen.ScrWrText(1, 1, message.c_str());
#endif
	BreakP = true;
	SPDLOG_WARN("GoExit(): Setting 'BreakP = true'");
	throw std::exception(message.c_str());
}

/// Spusti prikaz pres cmd.exe /c bez konzoloveho okna (CREATE_NO_WINDOW) a pocka na jeho konec.
/// Vstup i vystup jde do NUL, takze prikaz cekajici na klavesu (pause) nezustane viset.
static bool RunHiddenCommand(const std::string& cmd, int& exit_code)
{
	char comspec[MAX_PATH];
	DWORD len = GetEnvironmentVariableA("COMSPEC", comspec, sizeof(comspec));
	std::string shell = (len > 0 && len < sizeof(comspec)) ? comspec : "cmd.exe";
	std::string command_line = "\"" + shell + "\" /c " + cmd;

	SECURITY_ATTRIBUTES sa{ sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE };
	HANDLE nul_in = CreateFileA("NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, &sa, OPEN_EXISTING, 0, nullptr);
	HANDLE nul_out = CreateFileA("NUL", GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, &sa, OPEN_EXISTING, 0, nullptr);

	STARTUPINFOA si{};
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESTDHANDLES;
	si.hStdInput = nul_in;
	si.hStdOutput = nul_out;
	si.hStdError = nul_out;

	PROCESS_INFORMATION pi{};
	std::vector<char> buf(command_line.begin(), command_line.end());
	buf.push_back('\0');
	BOOL ok = CreateProcessA(nullptr, buf.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);

	if (nul_in != INVALID_HANDLE_VALUE) CloseHandle(nul_in);
	if (nul_out != INVALID_HANDLE_VALUE) CloseHandle(nul_out);

	if (!ok) {
		SPDLOG_ERROR("RunHiddenCommand(): CreateProcess failed, error {}", GetLastError());
		return false;
	}

	WaitForSingleObject(pi.hProcess, INFINITE);
	DWORD code = 0;
	GetExitCodeProcess(pi.hProcess, &code);
	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);
	exit_code = (int)code;
	return true;
}

bool OSshell(std::string path, std::string cmd_line, bool no_cancel, bool free_memory, bool load_font, bool text_mode)
{
	if (path == "FNDFILES.EXE") {
		LastExitCode = RunFndFilesExe(cmd_line);
	}
	else if (FandHost::IsEnabled()) {
		// okenni hostitel nema konzoli: _popen by pro cmd.exe otevrel nove konzolove okno
		std::string cmd = path.empty() ? cmd_line : path + " " + cmd_line;
		SPDLOG_INFO("OSshell() calling hidden command '{}'", cmd);
		if (!RunHiddenCommand(cmd, LastExitCode)) return false;
	}
	else {
		char psBuffer[128];
		FILE* pPipe;

		std::string cmd = path.empty() ? cmd_line : path + " " + cmd_line;
		SPDLOG_INFO("OSshell() calling command '{}'", cmd);

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

	uint8_t length = printer[printerNr].Strg[offset];
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
		DoneMouseEvents();
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

int32_t RunFndFilesExe(std::string cmd_line)
{
	std::vector<std::string> args;

	std::regex pattern("(\\\".*?\\\")|(\\S+)", std::regex_constants::icase);
	auto words_begin = std::sregex_iterator(cmd_line.begin(), cmd_line.end(), pattern);
	auto words_end = std::sregex_iterator();

	for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
		std::smatch match = *i;
		args.push_back(match.str());
	}

	if (args.size() != 3) {
		return 1; // bad params
	}

	std::vector<std::string> items = directoryItems(args[1]);
	std::string buf;
	for (size_t i = 0; i < items.size(); i++) {
		if (i > 0) buf += "\r\n";
		buf += items[i];
	}

	HANDLE output = OpenH(args[2], _isOverwriteFile, Exclusive);

	if (output == nullptr) {
		return 2; // blocked file
	}
	else {
		WriteH(output, buf.length(), buf.c_str());
		CloseH(&output);
	}

	return 0;
}
