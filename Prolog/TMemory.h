#pragma once
#include "../cppfand/Chained.h"
#include "../cppfand/constants.h"
#include "../Common/pstring.h"

struct TMemBlkHd : public Chained<TMemBlkHd>
{
	// TMemBlkHd* pChain = nullptr;
	WORD Sz = 0;
};

class TMemory {
public:
	TMemory();
	void Init();
	WORD RestSz;
	void* CurLoc;
	TMemBlkHd* CurBlk;
	TMemBlkHd* FreeList;
	void* Get(WORD Sz);
	void* Mark();
	void Release(void* P);
	pstring StoreStr(pstring s);
	void* Alloc(WORD Sz);
	void Free(void* P, WORD Sz);
};

extern TMemBlkHd* FreeMemList;
extern TMemory Mem1;
extern TMemory Mem2;
extern TMemory Mem3;
extern WORD ProlgCallLevel;