#pragma once
#include "XKey.h"

class XWKey : public XKey // r334
{
public:
	XWKey(FileD* parent);
	XWKey(FileD* parent, bool duplic, bool in_work, const std::vector<KeyFldD*>& key_fields);
	void Open(FileD* file_d, const std::vector<KeyFldD*>& key_fields, bool duplic, bool interval);
	void Close(FileD* file_d);
	void Release(FileD* file_d);
	void ReleaseTree(FileD* file_d, int Page, bool IsClose);
	void OneRecIdx(FileD* file_d, const std::vector<KeyFldD*>& key_fields, int N, void* record);
	void InsertAtNr(FileD* file_d, int I, int RecNr, void* record);
	int InsertGetNr(FileD* file_d, int RecNr, void* record);
	void DeleteAtNr(FileD* file_d, int I);
	void AddToRecNr(FileD* file_d, int RecNr, short Dif);
};
