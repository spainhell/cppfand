#pragma once
#include "constants.h"
#include "access.h"

void ScanSubstWIndex(XScan* Scan, KeyFldD* SK, char Typ);

void CreateIndexFile(); // r482
void CreateWIndex(XScan* Scan, WKeyDPtr K, char Typ); // r508
void SortAndSubst(KeyFldD* SK); // r534
void CopyIndex(WKeyDPtr K, KeyDPtr FromK); // r581
