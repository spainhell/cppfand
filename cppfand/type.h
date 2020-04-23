#pragma once
#include "constants.h"

double RealFromFix(void* FixNo, WORD FLen); // r30
void FixFromReal(double r, void* FixNo, WORD& flen); // r90

