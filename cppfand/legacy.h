#pragma once
#include "constants.h"
#include "pstring.h"


longint ExitCode = 0; // exit kód -> OS
void* ErrorAddr = nullptr; // adresa chyby
//void* ExitProc = nullptr;
void (*ExitProc)() { }; // ukončovací procedura

void val(pstring s, BYTE& b, WORD& err);
pstring copy(pstring source, size_t index, size_t count);
void str(int input, pstring& output);

void FSplit(pstring fullname, pstring& dir, pstring& name, pstring& ext);
pstring FSearch(pstring& path, pstring& dirlist);
pstring FExpand(pstring path);

void ChDir(pstring cesta);
void GetDir(BYTE disk, pstring& cesta);
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

inline void RunError(WORD code) { exit(code); }
inline void Halt(WORD code) { exit(code); }

void FillChar(char* cil, WORD delka, char vypln);
void Move(void* zdroj, void* cil, WORD delka);
BYTE Hi(WORD cislo);
BYTE Lo(WORD cislo);
WORD Swap(WORD cislo);

void UnPack(void* PackArr, WORD& NumArr, WORD& NoDigits);
void Pack(void* NumArr, WORD& PackArr, WORD& NoDigits);

char* GetEnv(const char* name);
