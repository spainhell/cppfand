#pragma once
#include "handle.h"

void ReleaseStore(void* pointer);
bool OSshell(pstring Path, string CmdLine, bool NoCancel, bool FreeMm, bool LdFont, bool TextMd);
void ClearCacheH(filePtr h);
void* GetZStore(WORD Size);
bool SaveCache(WORD ErrH);

