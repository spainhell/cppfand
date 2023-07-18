#pragma once

#include "../Common/FileEnums.h"
#include "../Common/LongStr.h"
#include <array>

#include "OldDrivers.h"
#include "../Common/pstring.h"

struct WRect { BYTE C1 = 0, R1 = 0, C2 = 0, R2 = 0; }; // r34
struct WordRec { BYTE Lo = 0, Hi = 0; };
struct LongRec { WORD Lo = 0, Hi = 0; };
typedef void* PProcedure;

bool IsLetter(char C); // r4
void MyMove(void* A1, void* A2, WORD N);

WORD ListLength(void* P); // r22 ASM
pstring StrPas(const char* Src);
void StrLPCopy(char* Dest, pstring s, WORD MaxL);
WORD SLeadEqu(pstring S1, pstring S2);
WORD SLeadEqu(const std::string& S1, const std::string& S2);
bool EqualsMask(void* p, WORD l, pstring Mask); // r86 ASM
bool EqualsMask(const std::string& value, std::string& mask);
bool EquLongStr(LongStr* S1, LongStr* S2);
bool EquArea(void* P1, void* P2, WORD L);
short MinI(short X, short Y);
short MaxI(short X, short Y);
WORD MinW(WORD X, WORD Y);
WORD MaxW(WORD X, WORD Y);
int MinL(int X, int Y);
int MaxL(int X, int Y);
bool OverlapByteStr(void* p1, void* p2); // ASM
WORD FindCtrlM(LongStr* s, WORD i, WORD n); // r152
WORD FindCtrlM(std::string& s, WORD i, WORD n);
WORD SkipCtrlMJ(LongStr* s, WORD i); // r158
WORD SkipCtrlMJ(std::string& s, WORD i);
void AddBackSlash(std::string& s);
void DelBackSlash(std::string& s);
bool MouseInRect(WORD X, WORD Y, WORD XSize, WORD Size); // r175 ASM
bool MouseInRectProc(WORD X, WORD Y, WORD XSize, WORD Size); // r182 ASM - rozdeleno na txt a graph rezim

// *** TIME, DATE ***
void SplitDate(double R, WORD& d, WORD& m, WORD& y);
double AddMonth(double R, double RM);
double DifMonth(double R1, double R2);
double ValDate(pstring Txt, pstring Mask);
pstring StrDate(double R, pstring Mask);
std::string CppToday();
double Today(); // r362
double CurrTime();

// *** DEBUGGING ***
void wait();
#ifndef FandRunV
pstring HexB(BYTE b);
pstring HexW(WORD i);
pstring HexD(int i);
pstring HexPtr(void* p);
void DispH(void* ad, short NoBytes);
#endif

void MarkStore(void* p);
void ReleaseStore(void** pointer);
void ReleaseAfterLongStr(void** pointer);
int StoreAvail();
std::string* StoreStr(std::string S);
void MarkBoth(void* p, void* p2);

void GoExit();
bool OSshell(std::string path, std::string cmd_line, bool no_cancel, bool free_memory, bool load_font, bool text_mode);


// ***  VIRTUAL HANDLES  ***
bool IsNetCVol();
bool CacheExist();
bool SaveCache(WORD ErrH, HANDLE f);
void SetUpdHandle(HANDLE H);
void ResetUpdHandle(HANDLE H);
bool IsUpdHandle(HANDLE H);
long SeekH(HANDLE handle, size_t pos);
long FileSizeH(HANDLE handle);
HANDLE OpenH(const std::string& path, FileOpenMode Mode, FileUseMode UM);
size_t ReadH(HANDLE handle, size_t length, void* buffer);
void WriteH(HANDLE handle, size_t length, void* buffer);
void FlushHandles();
void CloseH(HANDLE* handle);
void CloseClearH(HANDLE* h);
void RdWrCache(FileOperation operation, HANDLE handle, bool not_cached, size_t position, size_t count, void* buf);
void MyDeleteFile(pstring path);
void RenameFile56(pstring OldPath, pstring NewPath, bool Msg);
std::string MyFExpand(std::string Nm, std::string EnvName);
double RDate(WORD Y, WORD M, WORD D, WORD hh, WORD mm, WORD ss, WORD tt);

// *** DISPLAY ***
WORD LogToAbsLenStyleStr(pstring s, WORD l);

void SetMsgPar(std::string s);
void SetMsgPar(std::string s1, std::string s2);
void SetMsgPar(std::string s1, std::string s2, std::string s3);
void SetMsgPar(std::string s1, std::string s2, std::string s3, std::string s4);
std::string ReadMessage(int N);
void WriteMsg(WORD N);
void ClearLL(BYTE attr);

extern Spec spec;
extern Video video;
extern Fonts fonts;
std::string PrTab(WORD printerNr, WORD value);
void SetCurrPrinter(short NewPr);



typedef std::array<BYTE, 4> TPrTimeOut;

void OpenWorkH();

void NonameStartFunction();

