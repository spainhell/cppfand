#pragma once
#include "WorkFile.h"
#include "XKey.h"
#include "XScan.h"
#include "XXPage.h"


class XWorkFile : public WorkFile
{
public:
	XWorkFile(XScan* AScan, XKey* AK);
	XWorkFile(XScan* AScan, std::vector<XKey*>& AK);
	XXPage* PX = nullptr;
	XKey* KD = nullptr;
	XScan* Scan = nullptr;
	bool MsgWritten = false;
	longint NxtXPage = 0;
	XWFile* XF = nullptr;
	XPage* XPP = nullptr;
	void Main(char Typ);
	void CopyIndex(XKey* K, KeyFldD* KF, char Typ);
	bool GetCRec() override;
	void Output(WRec* R) override;
private:
	void FinishIndex();
};
