#pragma once
#include "handle.h"

void ClearCacheH(filePtr h) { return; } // r159
bool SaveCache(WORD ErrH) { return true; } // r142
void* GetStore(WORD Size) { return nullptr; } // r224
void* GetZStore(WORD Size) { return nullptr; } // r233
void ReleaseStore(void* pointer); // r275
void ReleaseAfterLongStr(void* pointer); // r293
void GoExit() { return; } // r350
bool OSshell(pstring Path, string CmdLine, bool NoCancel, bool FreeMm, bool LdFont, bool TextMd) { return true; } // r366
