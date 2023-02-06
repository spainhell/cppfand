#pragma once
#include "runfrml.h"

class TFile;

LongStr* GetTxt(FrmlElem* Z);
longint CopyTFFromGetTxt(TFile* TF, FrmlElem* Z);
longint CopyTFString(TFile* destT00File, FileD* srcFileDescr, TFile* scrT00File, longint srcT00Pos);
void CopyTFStringToH(FILE* h, TFile* TF02, FileD* TFD02, longint& TF02Pos);
