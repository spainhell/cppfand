#include "TMemory.h"

TMemBlkHd* FreeMemList = nullptr;
TMemory Mem1;
TMemory Mem2;
TMemory Mem3;
WORD ProlgCallLevel = 0;

const WORD MemBlockSize = 8192;

TMemory::TMemory()
{
	CurBlk = nullptr;
	CurLoc = nullptr;
	RestSz = 0;
	FreeList = nullptr;
}

void TMemory::Init()
{
	CurBlk = nullptr;
	CurLoc = nullptr;
	RestSz = 0;
	FreeList = nullptr;
}

void* TMemory::Get(WORD Sz)
{
	void* p = nullptr;
	TMemBlkHd* p1 = nullptr;
	TMemBlkHd* p2 = nullptr;
	WORD n = 0;
	if (Sz > RestSz) {
		n = Sz + sizeof(TMemBlkHd);
		if (n < MemBlockSize) n = MemBlockSize;
		p2 = (TMemBlkHd*)FreeMemList;
		p1 = FreeMemList;
		while (p1 != nullptr) {
			if (p1->Sz >= n) { p2->pChain = p1->pChain; goto label1; }
			p2 = p1; p1 = (TMemBlkHd*)p1->pChain;
		}
		p1 = new TMemBlkHd(); // GetStore2(n);
		p1->Sz = n;
	label1:
		p1->pChain = CurBlk;
		CurBlk = p1;
		//CurLoc = Ptr(Seg(p1^), Ofs(p1^) + sizeof(TMemBlkHd));
		RestSz = p1->Sz - sizeof(TMemBlkHd);
	}
	p = CurLoc;
	//PtrRec(CurLoc).Ofs += Sz;
	RestSz -= Sz;
	/*asm les di, p; mov al, 0; mov cx, Sz; cld; rep stosb;*/
	return p;
}

void* TMemory::Mark()
{
	return CurLoc;
}

void TMemory::Release(void* p) /* only for pure stack */
{
	TMemBlkHd* p1 = nullptr; TMemBlkHd* p2 = nullptr;
	p1 = CurBlk;
	/*while (PtrRec(p1).Seg != PtrRec(p).Seg) {
		p2 = p1->pChain;
		p1->pChain = FreeMemList;
		FreeMemList = p1;
		p1 = p2;
	}*/
	CurBlk = p1;
	CurLoc = p;
	if (p == nullptr) RestSz = 0;
	//else RestSz = p1->Sz - (PtrRec(p).Ofs - (PtrRec(p1).Ofs + sizeof(TMemBlkHd)));
}

pstring TMemory::StoreStr(pstring s)
{
	void* p;
	p = Get(s.length() + 1);
	memcpy(p, s.c_str(), s.length() + 1);
	//return p;
	return "";
}

void* TMemory::Alloc(WORD Sz) /* doesn't free once allocated blocks */
{
	TMemBlkHd* p = nullptr; TMemBlkHd* p1 = nullptr; TMemBlkHd* p2 = nullptr;
	Sz = (Sz + 7) & 0xfff8;
	p1 = (TMemBlkHd*)(&FreeList);
	p = FreeList;
	while (p != nullptr) {
		if (p->Sz >= Sz) {
			if (p->Sz > Sz) {
				p2 = p;
				//PtrRec(p2).Ofs += Sz;
				p1->pChain = p2;
				p2->pChain = p->pChain;
				p2->Sz = p->Sz - Sz;
			}
			else p1->pChain = p->pChain;
			/* asm les di, p; mov al, 0; mov cx, Sz; cld; rep stosb;*/
			return p;
		}
		p1 = p;
		p = (TMemBlkHd*)p->pChain;
	}
	return Get(Sz);
}

void TMemory::Free(void* P, WORD Sz)
{
	/*TMemBlkHd* p1 = nullptr;
	TMemBlkHd* p2 = nullptr;
	TMemBlkHd* pp = (TMemBlkHd*)P;
	Sz = (Sz + 7) & 0xfff8;
	p1 = (TMemBlkHd*)FreeList;
	p2 = FreeList;
	while ((PtrRec(p2).Seg != 0) && (PtrRec(p2).Seg != PtrRec(P).Seg)) {
		p1 = p2; p2 = p2->pChain;
	}
	while ((PtrRec(p2).Seg == PtrRec(P).Seg) && (PtrRec(p2).Ofs < PtrRec(P).Ofs)) {
		p1 = p2; p2 = p2->pChain;
	}
	if ((PtrRec(P).Seg == PtrRec(p2).Seg) && (PtrRec(P).Ofs + Sz == PtrRec(p2).Ofs)) {
		pp->Sz = Sz + p2->Sz; pp->pChain = p2->pChain;
	}
	else { pp->Sz = Sz; pp->pChain = p2; }
	if ((PtrRec(p1).Seg == PtrRec(P).Seg) && (PtrRec(p1).Ofs + p1->Sz == PtrRec(P).Ofs)) {
		p1->Sz = p1->Sz + pp->Sz; p1->pChain = pp->pChain;
	}
	else p1->pChain = pp;*/
}