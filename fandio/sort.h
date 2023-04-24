#pragma once
#include "XScan.h"

class Instr_getindex;

/* the index is sorted by key value && input order(IR) !! */
void ScanSubstWIndex(FileD* file_d, XScan* Scan, KeyFldD* SK, char Typ); // r518
void SortAndSubst(FileD* file_d, KeyFldD* SK); // r534
void GetIndexSort(Instr_getindex* PD);
void CopyIndex(FileD* file_d, XWKey* K, XKey* FromK); // r581
