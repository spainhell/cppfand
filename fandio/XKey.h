#pragma once
#include <string>
#include "XString.h"

class XWFile;
class XPage;

class XKey
{
public:
	XKey();
	XKey(const XKey& orig);
	XKey(unsigned char* inputStr);
	XKey* Chain = nullptr;
	KeyFldD* KFlds = nullptr;
	bool IntervalTest = false, Duplic = false, InWork = false;
	unsigned short IndexRoot = 0; unsigned char IndexLen = 0;
	int NR = 0; // {used only by XWKey}
	std::string Alias;
	XWFile* GetXFile();
	int NRecs();
	bool Search(XString& XX, bool AfterEqu, int& RecNr);
	bool Search(std::string X, bool AfterEqu, int& RecNr);
	bool SearchInterval(XString& XX, bool AfterEqu, int& RecNr);
	int PathToNr();
	void NrToPath(int I);
	int PathToRecNr();
	bool RecNrToPath(XString& XX, int RecNr);
	bool IncPath(unsigned short J, int& Pg);
	int NrToRecNr(int I);
	pstring NrToStr(int I);
	int RecNrToNr(int RecNr);
	//bool FindNr(XString& X, int& IndexNr);
	bool FindNr(std::string X, int& IndexNr);
	void InsertOnPath(XString& XX, int RecNr);
	void InsertNonLeafItem(XString& XX, XPage* P, XPage* UpP, int Page, unsigned short I, int& UpPage, unsigned int upSum, unsigned int downPage);
	void InsertLeafItem(XString& XX, XPage* P, XPage* UpP, int Page, unsigned short I, int RecNr, int& UpPage);
	void ChainPrevLeaf(XPage* P, int N);
	bool Insert(int RecNr, bool Try);
	void DeleteOnPath();
	void BalancePages(XPage* P1, XPage** P2, bool& Released);
	void XIDown(XPage* p, XPage* p1, unsigned short i, int& page1);
	bool Delete(int RecNr);
};

bool SearchKey(XString& XX, XKey* Key, int& NN);
int XNRecs(std::vector<XKey*>& K);
void TryInsertAllIndexes(int RecNr);
void DeleteXRec(int RecNr, bool DelT);
void OverWrXRec(int RecNr, void* P2, void* P);
