#pragma once
#include "access.h"

void NegateESDI(); // r50 ASM

/* CFile[->XF] */
void TestXFExist();
longint XNRecs(KeyDPtr K);
void RecallRec(longint RecNr);
void TryInsertAllIndexes(longint RecNr);
void DeleteAllIndexes(longint RecNr);
void DeleteXRec(longint RecNr, bool DelT);
void OverwrXRec(longint RecNr, void* P2, void* P);
void AddFFs(KeyDPtr K, pstring& s);
void CompKIFrml(KeyDPtr K, KeyInD* KI, bool AddFF);
