#include "memory.h"


#include "kbdww.h"
#include "oaccess.h"

void* Normalize(longint L)
{
	return nullptr;
}

longint AbsAdr(void* P)
{
	return 0;
}

void CloseXMS()
{
	if (XMSCachePages > 0)
	{
		// asm mov ah,0AH; mov dx,XMSHandle; call [XMSFun] ;
	}
}

void MoveToXMS(WORD NPage, void* Src)
{
}

void MoveFromXMS(WORD NPage, void* Dest)
{
}

bool CacheExist()
{
	return NCachePages > 0;
}

void SetMyHeapEnd()
{
	// MyHeapEnd = ptr(PtrRec(CacheEnd).Seg - CachePageSz, PtrRec(CacheEnd).Ofs);
}

void NewCachePage(CachePage* ZLast, CachePage* Z)
{
}

void FormatCache()
{
}

bool WrCPage(FILE* Handle, longint N, void* Buf, WORD ErrH)
{
	return true;
}

bool WriteCachePage(CachePage* Z, WORD ErrH)
{
	return true;
}

void ReadCachePage(CachePage* Z)
{
}

CachePage* Cache(BYTE Handle, longint Page)
{
	return nullptr;
}

void LockCache()
{
}

void UnLockCache()
{
}

bool SaveCache(WORD ErrH)
{
	return true;
}

void ClearCacheH(FILE* h)
{
}

void SubstHandle(WORD h1, WORD h2)
{
}

void FreeCachePage(CachePage* Z)
{
}

void ExpandCacheUp()
{
}

void ExpandCacheDown()
{
}

integer HeapErrFun(WORD Size)
{
	return 0;
}

void* GetStore(WORD Size)
{
	return nullptr;
}

void* GetZStore(WORD Size)
{
	return nullptr;
}

void AlignParagraph()
{
}

void* GetStore2(WORD Size)
{
	return nullptr;
}

void* GetZStore2(WORD Size)
{
	return nullptr;
}

pstring* StoreStr(pstring S)
{
	return nullptr;
}

void MarkStore(void* p)
{
}

void MarkStore2(void* p)
{
}

void MarkBoth(void* p, void* p2)
{
}

void ReleaseStore(void* pointer)
{
}

void ReleaseAfterLongStr(void* pointer)
{
}

void ReleaseStore2(void* p)
{
}

void ReleaseBoth(void* p, void* p2)
{
	ReleaseStore(p); ReleaseStore2(p2);
}

longint StoreAvail()
{
	return 512 * 1024 * 1024;
}

void AlignLongStr()
{
}

void NewExit(PProcedure POvr, ExitRecord* Buf)
{
}

void GoExit()
{
}

void RestoreExit(ExitRecord& Buf)
{
}

bool OSshell(pstring Path, pstring CmdLine, bool NoCancel, bool FreeMm, bool LdFont, bool TextMd)
{
	return true;
}
