#pragma once
#include "constants.h"
#include "../Common/pstring.h"
#include <vector>

extern std::vector<std::string> paramstr;

extern int ExitCode; // exit kód -> OS
extern void* ErrorAddr; // adresa chyby
extern void (*ExitProc)(); // ukončovací procedura

void val(pstring s, BYTE& b, WORD& err);
void val(pstring s, WORD& b, WORD& err);
void val(pstring s, short& b, short& err);
void val(pstring s, double& b, short& err);
void val(pstring s, double& b, WORD& err);
void val(pstring s, int& b, WORD& err);
void val(pstring s, int& b, short& err);
double valDouble(std::string& s, short& err);
pstring copy(pstring source, size_t index, size_t count);
void str(int input, pstring& output);
void str(double input, int total, int right, pstring& output);
void str(double input, int total, int right, std::string& output);
void str(double input, int right, pstring& output);
void str(double input, int right, std::string& output);

WORD pred(WORD input);
WORD succ(WORD input);

void FSplit(pstring fullname, pstring& dir, pstring& name, pstring& ext);
void FSplit(const std::string& fullname, std::string& dir, std::string& name, std::string& ext, char pathDelim = '\\');
pstring FSearch(pstring& path, pstring& dirlist);
std::string FSearch(const std::string path, const std::string dirlist);
std::string FExpand(std::string path, char pathDelim = '\\');

void ChDir(std::string cesta);
void GetDir(BYTE disk, pstring* cesta);
std::string GetDir(BYTE disk);
void MkDir(std::string cesta);
void RmDir(std::string cesta);
void Rename(std::string soubor, std::string novejmeno);
void Erase(std::string soubor);

void InitGraph(short GraphDriver, short GraphMode, pstring PathToDriver); // IGNORE
void CloseGraph(); // IGNORE

//double Random();
//WORD Random(WORD rozsah);

WORD ParamCount(); // vrací počet parametrů příkazové řádky
pstring ParamStr(short index);

inline void Exit() { return; }

//inline void RunError(WORD code) { exit(code); }
inline void Halt(WORD code) { exit(code); }

void FillChar(void* cil, int delka, size_t vypln);
void Move(void* zdroj, void* cil, WORD delka);
BYTE Hi(WORD cislo);
BYTE Lo(WORD cislo);
WORD Swap(WORD cislo);

inline void OvrInit(pstring FileName) {}
inline void OvrInitEMS() {}
inline int OvrGetBuf() { return 1024 * 1024; }
inline void OvrSetBuf(int Size) {}
inline void OvrSetRetry(int Size) {};

inline void GetMem(void* pointer, int Size) { pointer = new unsigned char[Size]; }

extern BYTE OvrResult; // vzdy 0 OvrOK

std::string GetEnv(const char* name);

WORD IOResult();
WORD DosError();
double DiskFree(char disk);

//typedef void* HANDLE;

class TextFile
{
public:
	~TextFile();
	FILE* Handle = nullptr;
	std::string Mode = ""; // read, write, append ...
	size_t bufsize = 0;
	size_t _private = 0;
	size_t bufpos = 0;
	size_t bufend = 0;
	short (*openfunc)(TextFile* F) = nullptr; // function pointer
	short (*inoutfunc)(TextFile* F) = nullptr; // function pointer
	short (*flushfunc)(TextFile* F) = nullptr; // function pointer
	short (*closefunc)(TextFile* F) = nullptr; // function pointer
	short (*opentxt)(TextFile* F) = nullptr; // function pointer
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
