#pragma once
#include "XScan.h"

class Instr_getindex;

/* the index is sorted by key value && input order(IR) !! */
void CreateWIndex(XScan* Scan, XWKey* K, char Typ); // r508
void ScanSubstWIndex(XScan* Scan, KeyFldD* SK, char Typ); // r518
void SortAndSubst(KeyFldD* SK); // r534
void GetIndexSort(Instr_getindex* PD);
void CopyIndex(XWKey* K, XKey* FromK); // r581
