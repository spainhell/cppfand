#pragma once

void ReleaseStore(void* pointer);
bool OSshell(PathStr Path, string CmdLine, bool NoCancel, bool FreeMm, bool LdFont, bool TextMd);
void ClearCacheH(WORD h);
void* GetZStore(WORD Size);
bool SaveCache(WORD ErrH);