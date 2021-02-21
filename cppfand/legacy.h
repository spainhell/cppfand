#pragma once
#include "constants.h"
#include "pstring.h"
#include <vector>

extern std::vector<std::string> paramstr;

extern longint ExitCode; // exit kód -> OS
extern void* ErrorAddr; // adresa chyby
extern void (*ExitProc)(); // ukončovací procedura

void val(pstring s, BYTE& b, WORD& err);
void val(pstring s, WORD& b, WORD& err);
void val(pstring s, integer& b, integer& err);
void val(pstring s, double& b, integer& err);
void val(pstring s, double& b, WORD& err);
void val(pstring s, longint& b, WORD& err);
double valDouble(std::string& s, integer& err);
pstring copy(pstring source, size_t index, size_t count);
void str(int input, pstring& output);
void str(double input, int total, int right, pstring& output);
void str(double input, int total, int right, std::string& output);
void str(double input, int right, pstring& output);
void str(double input, int right, std::string& output);

WORD pred(WORD input);
WORD succ(WORD input);

void FSplit(pstring fullname, pstring& dir, pstring& name, pstring& ext);
void FSplit(const std::string& fullname, std::string& dir, std::string& name, std::string& ext);
pstring FSearch(pstring& path, pstring& dirlist);
std::string FExpand(std::string path);

void ChDir(std::string cesta);
void GetDir(BYTE disk, pstring* cesta);
pstring GetDir(BYTE disk);
void MkDir(std::string cesta);
void RmDir(std::string cesta);
void Rename(std::string soubor, std::string novejmeno);
void Erase(std::string soubor);

void InitGraph(short GraphDriver, short GraphMode, pstring PathToDriver); // IGNORE
void CloseGraph(); // IGNORE

//double Random();
//WORD Random(WORD rozsah);

WORD ParamCount(); // vrací počet parametrů příkazové řádky
pstring ParamStr(integer index);

inline void Exit() { return; }

//inline void RunError(WORD code) { exit(code); }
inline void Halt(WORD code) { exit(code); }

void FillChar(void* cil, int delka, size_t vypln);
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

extern BYTE OvrResult; // vždy 0 OvrOK

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
	integer (*openfunc)(TextFile* F) = nullptr; // function pointer
	integer (*inoutfunc)(TextFile* F) = nullptr; // function pointer
	integer (*flushfunc)(TextFile* F) = nullptr; // function pointer
	integer (*closefunc)(TextFile* F) = nullptr; // function pointer
	integer (*opentxt)(TextFile* F) = nullptr; // function pointer
	BYTE UserData[32] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	std::string name;
	std::string LineEnd;
	std::string FullPath;
	BYTE* buffer = nullptr;
	bool eof = false;

	const char* c_str();
	void Close(const char* data);
	void Assign(std::string FullPath);
	void Reset();
	void Rewrite();
	bool ResetTxt();
};
