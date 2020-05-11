#pragma once
#include "runfrml.h"

LongStr* GetTxt(FrmlPtr Z);
longint CopyTFFromGetTxt(TFile* TF, FrmlElem* Z);
longint CopyTFString(TFilePtr TF, FileDPtr FD2, TFilePtr TF2, longint Pos2);
void CopyTFStringToH(FILE* h);
