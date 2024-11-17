#pragma once
#include "WorkFile.h"
#include "XKey.h"
#include "XScan.h"
#include "XXPage.h"

class XWorkFile : public WorkFile
{
public:
	XWorkFile(FileD* parent, XScan* AScan, std::vector<XKey*>& AK);

	void Main(char Typ, void* record);
	void CopyIndex(XKey* K, std::vector<KeyFldD*>& KF, char Typ, void* record);
	bool GetCRec(void* record) override;
	void Output(XKey* xKey, WRec* R, void* record) override;

	XWFile* xwFile = nullptr;
	XPage* xPage = nullptr;
	int nextXPage = 0;
	bool msgWritten = false;

private:
	void FinishIndex();

	std::vector<XKey*> x_keys_;
	XXPage* xxPage = nullptr;
	XScan* xScan = nullptr;
};
