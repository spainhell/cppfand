#pragma once

#include "../Common/pstring.h"
#include "../Common/typeDef.h"
#include "../Drivers/host.h"
#include <vector>

extern std::vector<std::string> paramstr;

extern int ExitCode; // exit kód -> OS
extern void* ErrorAddr; // adresa chyby
extern void (*ExitProc)(); // ukonèovací procedura

// val, str, copy, pred, succ, FSplit, FillChar, Hi, Lo, Swap
#include "../fandbase/pascal.h"

pstring FSearch(pstring& path, pstring& dirlist);
std::string FSearch(const std::string path, const std::string dirlist);
std::string FExpand(std::string path, char pathDelim = '\\');

void ChDir(std::string cesta);
void GetDir(uint8_t disk, pstring* cesta);
std::string GetDir(uint8_t disk);
void MkDir(std::string cesta);
void RmDir(std::string cesta);
void Rename(std::string soubor, std::string novejmeno);
void Erase(std::string soubor);

void InitGraph(short GraphDriver, short GraphMode, pstring PathToDriver); // IGNORE
void CloseGraph(); // IGNORE

//double Random();
//WORD Random(WORD rozsah);

WORD ParamCount(); // vrací poèet parametrù pøíkazové øádky

inline void Exit() { return; }

//inline void RunError(WORD code) { exit(code); }
inline void Halt(WORD code) { if (FandHost::IsEnabled()) throw FandHost::HaltException(code); exit(code); }


inline void GetMem(void* pointer, int Size) { pointer = new unsigned char[Size]; }


WORD IOResult();
WORD DosError();

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
	uint8_t UserData[32] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	std::string name;
	std::string LineEnd;
	std::string FullPath;
	uint8_t* buffer = nullptr;
	bool eof = false;

	const char* c_str();
	void Close(const char* data);
	void Assign(std::string FullPath);
	void Reset();
	void Rewrite();
	bool ResetTxt();
};
