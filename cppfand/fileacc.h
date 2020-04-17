#pragma once
#include "access.h"

using namespace std;

void WrPrefix();
void WrDBaseHd();
void OldMode(LockMode Mode);
bool ChangeLMode(LockMode Mode, WORD Kind, bool RdPref);
LockMode NewLMode(LockMode Mode);
void OldLMode(LockMode Mode);
void WrPrefixes();
void CExtToX();
void CExtToT();
