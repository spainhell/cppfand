#pragma once
#include "constants.h"

BYTE ByteMask[_MAX_INT_DIG];

void UnPack(void* PackArr, void* NumArr, WORD NoDigits); // ASM
void Pack(void* NumArr, void* PackArr, WORD NoDigits);

const BYTE DblS = 8;
const BYTE FixS = 8;
BYTE Fix[FixS];
BYTE RealMask[DblS + 1];
BYTE Dbl[DblS];

double RealFromFix(void* FixNo, WORD FLen); // r30
void FixFromReal(double r, void* FixNo, WORD& flen); // r90

