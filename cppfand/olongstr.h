#pragma once
#include "runfrml.h"

class FandTFile;

LongStr* GetTxt(FrmlElem* Z);
int CopyTFFromGetTxt(FandTFile* TF, FrmlElem* Z);
int CopyTFString(FandTFile* destT00File, FileD* srcFileDescr, FandTFile* scrT00File, int srcT00Pos);
void CopyTFStringToH(FILE* h, FandTFile* TF02, FileD* TFD02, int& TF02Pos);
