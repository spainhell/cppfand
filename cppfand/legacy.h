#pragma once
#include <string>
#include "constants.h"

using namespace std;

void val(string s, BYTE& b, WORD& err);
string copy(string source, size_t index, size_t count);

void FSplit(string fullname, string& dir, string& name, string& ext);
string FExpand(string path);

void ChDir(string cesta);
void GetDir(BYTE disk, string& cesta);
void MkDir(string cesta);
void RmDir(string cesta);
void Rename(string soubor, string novejmeno);
void Erase(string soubor);

float Random();
WORD Random(WORD rozsah);

WORD ParamCount(); // vrací poèet parametrù pøíkaz. øádky
string ParamStr(integer index);

inline void Exit() { return;; }

inline void RunError(WORD c) { exit(c); }
inline void Halt(WORD c) { RunError(c); }

void FillChar(char* cil, WORD delka, char vypln);
void Move(void* zdroj, void* cil, WORD delka);
BYTE Hi(WORD cislo);
BYTE Lo(WORD cislo);
WORD Swap(WORD cislo);

void UnPack(void* PackArr, WORD& NumArr, WORD& NoDigits);
void Pack(void* NumArr, WORD& PackArr, WORD& NoDigits);
