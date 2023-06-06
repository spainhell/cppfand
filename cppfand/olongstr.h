#pragma once
#include "runfrml.h"

class FandTFile;

LongStr* GetTxt(FileD* file_d, FrmlElem* Z, void* record);
int CopyTFFromGetTxt(FileD* file_d, FandTFile* TF, FrmlElem* Z, void* record);
int CopyTFString(FileD* file_d, FandTFile* destT00File, FileD* srcFileDescr, FandTFile* scrT00File, int srcT00Pos);
void CopyTFStringToH(FileD* file_d, HANDLE h, FandTFile* TF02, FileD* TFD02, int& TF02Pos);
