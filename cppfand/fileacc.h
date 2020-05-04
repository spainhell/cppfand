#pragma once
#include "access.h"

using namespace std;

void ResetCFileUpdH();
void ClearCacheCFile();
void CloseClearHCFile();

void WrPrefix();
void WrDBaseHd();
void OldMode(LockMode Mode);
bool TryLockN(longint N, WORD Kind); // r100
void UnLockN(longint N); // r119
bool ChangeLMode(LockMode Mode, WORD Kind, bool RdPref);
bool TryLMode(LockMode Mode, LockMode& OldMode, WORD Kind);
LockMode NewLMode(LockMode Mode);
void OldLMode(LockMode Mode);
void WrPrefixes();
void CExtToX();
void CExtToT();
void TestCFileError();
void TestCPathError();
void CloseGoExit();
void XFNotValid();
WORD RdPrefix();
void RdPrefixes();

