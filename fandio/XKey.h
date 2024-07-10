#pragma once
#include <string>
#include "XString.h"

class FileD;
class XWFile;
class XPage;

class XKey
{
public:
	XKey(FileD* parent);
	XKey(const XKey& orig);

	std::string Alias;
	std::vector<KeyFldD*> KFlds;
	bool IntervalTest = false;
	bool Duplic = false;
	bool InWork = false;
	unsigned short IndexRoot = 0;
	unsigned char IndexLen = 0;
	int NR = 0; // { used only by XWKey }

	XWFile* GetXFile(FileD* file_d);
	int NRecs();
	bool Search(FileD* file_d, XString& XX, bool AfterEqu, int& RecNr);
	bool Search(FileD* file_d, std::string X, bool AfterEqu, int& RecNr);
	bool SearchInterval(FileD* file_d, XString& XX, bool AfterEqu, int& RecNr);
	int PathToNr(FileD* file_d);
	void NrToPath(FileD* file_d, int I);
	int PathToRecNr(FileD* file_d);
	bool RecNrToPath(FileD* file_d, XString& XX, int RecNr, void* record);
	bool IncPath(FileD* file_d, unsigned short J, int& Pg);
	int NrToRecNr(FileD* file_d, int I);
	std::string NrToStr(FileD* file_d, int I);
	int RecNrToNr(FileD* file_d, int RecNr, void* record);
	bool FindNr(FileD* file_d, std::string X, int& IndexNr);
	void InsertOnPath(FileD* file_d, XString& XX, int RecNr);
	void InsertNonLeafItem(FileD* file_d, XString& XX, XPage* P, XPage* UpP, int Page, unsigned short I, int& UpPage, unsigned int upSum, unsigned int downPage);
	void InsertLeafItem(FileD* file_d, XString& XX, XPage* P, XPage* UpP, int Page, unsigned short I, int RecNr, int& UpPage);
	void ChainPrevLeaf(FileD* file_d, XPage* P, int N);
	bool Insert(FileD* file_d, int RecNr, bool Try, void* record);
	void DeleteOnPath(FileD* file_d);
	void BalancePages(XPage* P1, XPage** P2, bool& Released);
	void XIDown(FileD* file_d, XPage* p, XPage* p1, unsigned short i, int& page1);
	bool Delete(FileD* file_d, int RecNr, void* record);

protected:
	void CalcIndexLen();

private:
	FileD* parent_;
};
