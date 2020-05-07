#pragma once
#include "access.h"
#include "rdrun.h"

/* the index is sorted by key value && input order(IR) !! */

void CreateIndexFile(); // r482
void CreateWIndex(XScan* Scan, WKeyDPtr K, char Typ); // r508
void ScanSubstWIndex(XScan* Scan, KeyFldD* SK, char Typ); // r518
void SortAndSubst(KeyFldD* SK); // r534
//void GetIndexA(Instr* PD);
void CopyIndex(WKeyDPtr K, KeyDPtr FromK); // r581
