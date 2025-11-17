#pragma once

#include "../Common/LongStr.h"
#include <array>
#include "../Common/pstring.h"
#include "../fandio/FileEnums.h"
#include "datafiles.h"
#include "OldDrivers.h"

class XWFile;
class FandTFile;
class FandXFile;
class Fand0File;

struct WRect { uint8_t C1 = 0, R1 = 0, C2 = 0, R2 = 0; }; // r34
struct WordRec { uint8_t Lo = 0, Hi = 0; };
struct LongRec { WORD Lo = 0, Hi = 0; };
typedef void* PProcedure;

bool IsLetter(char C); // r4
void MyMove(void* A1, void* A2, WORD N);

//WORD SLeadEqu(pstring S1, pstring S2);
//WORD SLeadEqu(const std::string& S1, const std::string& S2);
bool EqualsMask(void* p, WORD l, pstring Mask); // r86 ASM
bool EqualsMask(const std::string& value, std::string& mask);
bool EquArea(void* p1, void* p2, size_t len);
short MinI(short X, short Y);
short MaxI(short X, short Y);
WORD MinW(WORD X, WORD Y);
WORD MaxW(WORD X, WORD Y);
int MinL(int X, int Y);
int MaxL(int X, int Y);

WORD FindCtrlM(std::string& s, WORD i, WORD n);
WORD SkipCtrlMJ(std::string& s, WORD i);
void AddBackSlash(std::string& s);
void DelBackSlash(std::string& s);
bool MouseInRect(WORD X, WORD Y, WORD XSize, WORD Size); // r175 ASM
bool MouseInRectProc(WORD X, WORD Y, WORD XSize, WORD Size); // r182 ASM - rozdeleno na txt a graph rezim

// *** DEBUGGING ***
void wait();
#ifndef FandRunV
pstring HexB(uint8_t b);
pstring HexW(WORD i);
pstring HexD(int i);
pstring HexPtr(void* p);
void DispH(void* ad, short NoBytes);
#endif

void MarkStore(void* p);
void ReleaseStore(void** pointer);
void ReleaseAfterLongStr(void** pointer);
int MemoryAvailable();
void MarkBoth(void* p, void* p2);

void GoExit(const std::string& message);
bool OSshell(std::string path, std::string cmd_line, bool no_cancel, bool free_memory, bool load_font, bool text_mode);


// ***  VIRTUAL HANDLES  ***
bool IsNetCVol();
bool CacheExist();
bool SaveCache(WORD ErrH, HANDLE f);

long SeekH(HANDLE handle, size_t pos);
long FileSizeH(HANDLE handle);
HANDLE OpenH(const std::string& path, FileOpenMode Mode, FileUseMode UM);
size_t ReadH(HANDLE handle, size_t length, void* buffer);
size_t WriteH(HANDLE handle, size_t length, const void* buffer);
void CloseH(HANDLE* handle);
void CloseClearH(HANDLE* h);

void MyDeleteFile(const std::string& path);
void RenameFile56(const std::string& OldPath, const std::string& NewPath, bool Msg);
std::string MyFExpand(std::string Nm, std::string EnvName);


// *** DISPLAY ***
void SetMsgPar(const std::string& s);
void SetMsgPar(const std::string& s1, const std::string& s2);
void SetMsgPar(const std::string& s1, const std::string& s2, const std::string& s3);
void SetMsgPar(const std::string& s1, const std::string& s2, const std::string& s3, const std::string& s4);
std::string ReadMessage(int N);
void WriteMsg(WORD N);

extern Spec spec;
extern Video video;
extern Fonts fonts;
std::string PrTab(WORD printerNr, WORD value);
void SetCurrPrinter(short NewPr);

typedef std::array<uint8_t, 4> TPrTimeOut;

void OpenWorkH();

void NonameStartFunction();


// UCTOxx methods
int32_t RunFndFilesExe(std::string cmd_line);
