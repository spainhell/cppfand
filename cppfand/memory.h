#pragma once
#include "handle.h"

void* Normalize(longint L); // ASM
longint AbsAdr(void* P); // ASM
void CloseXMS(); // ASM
void MoveToXMS(WORD NPage, void* Src); // ASM
void MoveFromXMS(WORD NPage, void* Dest); // ASM
bool CacheExist(); // r42
void SetMyHeapEnd();
void NewCachePage(CachePage* ZLast, CachePage* Z);
void FormatCache();
bool WrCPage(FILE* Handle, longint N, void* Buf, WORD ErrH);
bool WriteCachePage(CachePage* Z, WORD ErrH);
void ReadCachePage(CachePage* Z);
CachePage* Cache(BYTE Handle, longint Page);
void LockCache();
void UnLockCache();
bool SaveCache(WORD ErrH); // r142
void ClearCacheH(FILE* h); // r159
void SubstHandle(WORD h1, WORD h2);
void FreeCachePage(CachePage* Z);
void ExpandCacheUp();
void ExpandCacheDown();
integer HeapErrFun(WORD Size);
void* GetStore(WORD Size); // r224 ASM
void* GetZStore(WORD Size); // r233 ASM
void AlignParagraph();
void* GetStore2(WORD Size); // r224
void* GetZStore2(WORD Size); // r255 ASM
pstring* StoreStr(pstring S); // r261 ASM
void MarkStore(void* p); // r268
void MarkStore2(void* p); // r270
void MarkBoth(void* p, void* p2); // r272
void ReleaseStore(void* pointer); // r275
void ReleaseAfterLongStr(void* pointer); // r293
void ReleaseStore2(void* p); // r298
void ReleaseBoth(void* p, void* p2); // r305
longint StoreAvail(); // r309
void AlignLongStr(); // r314
void NewExit(PProcedure POvr, ExitRecord* Buf);
void GoExit(); // r350
void RestoreExit(ExitRecord& Buf); // r362
bool OSshell(pstring Path, pstring CmdLine, bool NoCancel,
	bool FreeMm, bool LdFont, bool TextMd); // r366
