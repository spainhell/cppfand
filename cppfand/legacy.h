#pragma once
#include "constants.h"
#include "pstring.h"

static longint ExitCode = 0; // exit kód -> OS
static void* ErrorAddr = nullptr; // adresa chyby
static void (*ExitProc)() { }; // ukončovací procedura

void val(pstring s, BYTE& b, WORD& err);
void val(pstring s, WORD& b, WORD& err);
void val(pstring s, integer& b, integer& err);
void val(pstring s, double& b, integer& err);
void val(pstring s, double& b, WORD& err);
void val(pstring s, longint& b, WORD& err);
pstring copy(pstring source, size_t index, size_t count);
void str(int input, pstring& output);
void str(double input, int total, int right, pstring& output);
void str(double input, int right, pstring& output);

WORD pred(WORD input);
WORD succ(WORD input);

void FSplit(pstring fullname, pstring& dir, pstring& name, pstring& ext);
pstring FSearch(pstring& path, pstring& dirlist);
pstring FExpand(pstring path);

void ChDir(pstring cesta);
void GetDir(BYTE disk, pstring* cesta);
pstring GetDir(BYTE disk);
void MkDir(pstring cesta);
void RmDir(pstring cesta);
void Rename(pstring soubor, pstring novejmeno);
void Erase(pstring soubor);

void InitGraph(short GraphDriver, short GraphMode, pstring PathToDriver); // IGNORE
void CloseGraph(); // IGNORE

double Random();
WORD Random(WORD rozsah);

WORD ParamCount(); // vrací počet parametrů příkazové řádky
pstring ParamStr(integer index);

inline void Exit() { return;; }

//inline void RunError(WORD code) { exit(code); }
inline void Halt(WORD code) { exit(code); }

void FillChar(void* cil, WORD delka, char vypln);
void Move(void* zdroj, void* cil, WORD delka);
BYTE Hi(WORD cislo);
BYTE Lo(WORD cislo);
WORD Swap(WORD cislo);

void UnPack(void* PackArr, WORD& NumArr, WORD& NoDigits);
void Pack(void* NumArr, WORD& PackArr, WORD& NoDigits);
inline void OvrInit(pstring FileName) {}
inline void OvrInitEMS() {}
inline longint OvrGetBuf() { return 1024 * 1024; }
inline void OvrSetBuf(longint Size) {}
inline void OvrSetRetry(longint Size) {};

inline void GetMem(void* pointer, int Size) { pointer = new unsigned char[Size]; }

void beep();

static BYTE OvrResult = 0; // vždy 0 OvrOK

pstring GetEnv(const char* name);

WORD IOResult();
WORD DosError();
double DiskFree(char disk);

class TextFile
{
public:
	FILE* Handle = nullptr;
	std::string Mode = ""; // read, write, append ...
	size_t bufsize = 0;
	size_t _private = 0;
	size_t bufpos = 0;
	size_t bufend = 0;
	void* openfunc = nullptr;
	void* inoutfunc = nullptr;
	void* flushfunc = nullptr;
	void* closefunc = nullptr;
	void* opentxt = nullptr;
	BYTE UserData[32] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
	std::string name;
	std::string LineEnd;
	BYTE* buffer = nullptr;
	bool eof;

	const char* c_str();
	void Close();
	void Assign(const char *);
	void Reset();
	void Rewrite();
	bool ResetTxt();
};
