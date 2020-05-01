#pragma once
#include "constants.h"
#include "access.h"

void ScanSubstWIndex(XScan* Scan, KeyFldD* SK, char Typ);

void CreateIndexFile(); // r482
void CopyIndex(WKeyDPtr K, KeyDPtr FromK); // r581
