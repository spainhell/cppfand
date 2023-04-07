#pragma once
#include "XKey.h"

class XWKey : public XKey // r334
{
public:
	void Open(KeyFldD* KF, bool Dupl, bool Intvl);
	void Close();
	void Release();
	void ReleaseTree(int Page, bool IsClose);
	void OneRecIdx(KeyFldD* KF, int N);
	void InsertAtNr(int I, int RecNr);
	int InsertGetNr(int RecNr);
	void DeleteAtNr(int I);
	void AddToRecNr(int RecNr, short Dif);
};
typedef XWKey* WKeyDPtr;
