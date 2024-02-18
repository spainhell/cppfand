#pragma once
#include "runfrml.h"

class FandTFile;

LongStr* GetTxt(FileD* file_d, FrmlElem16* Z, void* record);
int CopyTFFromGetTxt(FileD* file_d, FandTFile* TF, FrmlElem16* Z, void* record);
