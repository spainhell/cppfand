#pragma once
#include "access.h"

void ResetCFileUpdH();
void ClearCacheCFile();
void CloseClearHCFile();

#ifdef FandNetV
const longint TransLock = 0x0A000501;  /* locked while state transition */
const longint ModeLock = 0x0A000000;  /* base for mode locking */
const longint RecLock = 0x0B000000;  /* base for record locking */
void ModeLockBnds(LockMode Mode, longint& Pos, WORD& Len);
bool ChangeLMode(LockMode Mode, WORD Kind, bool RdPref);
#else
bool ChangeLMode(LockMode Mode, WORD Kind, bool RdPref);
#endif

bool TryLMode(LockMode Mode, LockMode& OldMode, WORD Kind);
LockMode NewLMode(LockMode Mode);
void OldLMode(LockMode Mode);
bool TryLockN(longint N, WORD Kind); // r100
void UnLockN(longint N); // r119
void CExtToT();
void CExtToX();
void CloseGoExit();
void TestCFileError();
void TestCPathError();

struct TT1Page
{
	WORD Signum = 0, OldMaxPage = 0;
	longint FreePart = 0;
	bool Rsrvd1 = false, CompileProc = false, CompileAll = false;
	WORD IRec = 0;
	longint FreeRoot = 0, MaxPage = 0;   /*eldest version=>array Pw[1..40] of char;*/
	double TimeStmp = 0.0;
	bool HasCoproc = false;
	char Rsrvd2[25];
	char Version[4];
	BYTE LicText[105];
	BYTE Sum = 0;
	char X1[295];
	WORD LicNr = 0;
	char X2[11];
	char PwNew[40];
	BYTE Time = 0;
};

WORD RdPrefix();
void RdPrefixes();
void WrPrefix();
void WrDBaseHd();
void WrPrefixes();
void XFNotValid();
