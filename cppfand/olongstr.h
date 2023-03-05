#pragma once
#include "runfrml.h"

class FandTFile;

LongStr* GetTxt(FrmlElem* Z);
longint CopyTFFromGetTxt(FandTFile* TF, FrmlElem* Z);
longint CopyTFString(FandTFile* destT00File, FileD* srcFileDescr, FandTFile* scrT00File, longint srcT00Pos);
void CopyTFStringToH(FILE* h, FandTFile* TF02, FileD* TFD02, longint& TF02Pos);
