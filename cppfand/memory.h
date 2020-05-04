#pragma once
#include "handle.h"

void ClearCacheH(filePtr h) { return; } // r159
bool SaveCache(WORD ErrH) { return true; } // r142
void* GetStore(WORD Size) { return nullptr; } // r224 ASM
void* GetZStore(WORD Size) { return nullptr; } // r233 ASM
void* GetStore2(WORD Size) { return nullptr; } // r224
void* GetZStore2(WORD Size) { return nullptr; } // r255 ASM
pstring* StoreStr(pstring S) { return nullptr; } // r261 ASM
void MarkStore(void* p) {} // r268
void MarkStore2(void* p) {} // r270
void MarkBoth(void* p, void* p2) {} // r272
void ReleaseStore(void* pointer) {}; // r275
void ReleaseAfterLongStr(void* pointer) {}; // r293
void ReleaseStore2(void* p); // r298
void ReleaseBoth(void* p, void* p2) { ReleaseStore(p); ReleaseStore2(p2); } // r305
longint StoreAvail() { return 512*1024*1024; } // r309
void AlignLongStr(); // r314
void GoExit() { return; } // r350
void RestoreExit(ExitRecord& Buf) {}; // r362
bool OSshell(pstring Path, pstring CmdLine, bool NoCancel, 
	bool FreeMm, bool LdFont, bool TextMd) { return true; } // r366
