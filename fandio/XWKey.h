#pragma once
#include "../cppfand/constants.h"
#include "XKey.h"

class XWKey : public XKey // r334
{
public:
	void Open(KeyFldD* KF, bool Dupl, bool Intvl);
	void Close();
	void Release();
	void ReleaseTree(longint Page, bool IsClose);
	void OneRecIdx(KeyFldD* KF, longint N);
	void InsertAtNr(longint I, longint RecNr);
	longint InsertGetNr(longint RecNr);
	void DeleteAtNr(longint I);
	void AddToRecNr(longint RecNr, integer Dif);
};
typedef XWKey* WKeyDPtr;
