#pragma once
#include "XKey.h"

class XWKey : public XKey // r334
{
public:
	XWKey(FileD* parent);
	void Open(FileD* file_d, KeyFldD* KF, bool Dupl, bool Intvl);
	void Close(FileD* file_d);
	void Release(FileD* file_d);
	void ReleaseTree(FileD* file_d, int Page, bool IsClose);
	void OneRecIdx(FileD* file_d, KeyFldD* KF, int N, void* record);
	void InsertAtNr(FileD* file_d, int I, int RecNr, void* record);
	int InsertGetNr(FileD* file_d, int RecNr, void* record);
	void DeleteAtNr(FileD* file_d, int I);
	void AddToRecNr(FileD* file_d, int RecNr, short Dif);
};
