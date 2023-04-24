#pragma once
#include "WorkFile.h"
#include "XKey.h"
#include "XScan.h"
#include "XXPage.h"

class XWorkFile : public WorkFile
{
public:
	XWorkFile(FileD* parent, XScan* AScan, XKey* AK);
	XXPage* xxPage = nullptr;
	XKey* xKey = nullptr;
	XScan* xScan = nullptr;
	bool msgWritten = false;
	int nextXPage = 0;
	XWFile* xwFile = nullptr;
	XPage* xPage = nullptr;
	void Main(char Typ, void* record);
	void CopyIndex(XKey* K, KeyFldD* KF, char Typ, void* record);
	bool GetCRec(void* record) override;
	void Output(WRec* R, void* record) override;
private:
	
	void FinishIndex();
};
