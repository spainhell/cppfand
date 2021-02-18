#pragma once
#include "runfrml.h"

class TFile;

LongStr* GetTxt(FrmlPtr Z);
longint CopyTFFromGetTxt(TFile* TF, FrmlElem* Z);
longint CopyTFString(TFile* TF, FileD* FD2, TFile* TF2, longint Pos2);
void CopyTFStringToH(FILE* h);
