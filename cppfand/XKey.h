#pragma once
#include <string>
#include "constants.h"
#include "pstring.h"
#include "XItemNonLeaf.h"
#include "XString.h"

class XWFile;
class XPage;

class XKey // r309
{
public:
	XKey();
	XKey(const XKey& orig, bool copyFlds);
	XKey(BYTE* inputStr);
	XKey* Chain = nullptr;
	KeyFldD* KFlds = nullptr;
	bool Intervaltest = false, Duplic = false, InWork = false;
	WORD IndexRoot = 0; BYTE IndexLen = 0;
	longint NR = 0; // {used only by XWKey}
	std::string* Alias = nullptr;
	XWFile* XF();
	longint NRecs();
	bool Search(XString& XX, bool AfterEqu, longint& RecNr);
	bool Search(std::string X, bool AfterEqu, longint& RecNr);
	bool SearchIntvl(XString& XX, bool AfterEqu, longint& RecNr);
	longint PathToNr();
	void NrToPath(longint I);
	longint PathToRecNr();
	bool RecNrToPath(XString& XX, longint RecNr);
	bool IncPath(WORD J, longint& Pg);
	longint NrToRecNr(longint I);
	pstring NrToStr(longint I);
	longint RecNrToNr(longint RecNr);
	bool FindNr(XString& X, longint& IndexNr);
	bool FindNr(std::string X, longint& IndexNr);
	void InsertOnPath(XString& XX, longint RecNr);
	void InsertNonLeafItem(XString& XX, XPage* P, XPage* UpP, longint Page, WORD I, longint& UpPage, unsigned int upSum, unsigned int downPage);
	void InsertLeafItem(XString& XX, XPage* P, XPage* UpP, longint Page, WORD I, int RecNr, longint& UpPage);
	void ChainPrevLeaf(XPage* P, longint N);
	bool Insert(longint RecNr, bool Try);
	void DeleteOnPath();
	void BalancePages(XPage* P1, XPage** P2, bool& Released);
	void XIDown(XPage* P, XPage* P1, WORD I, longint& Page1);
	bool Delete(longint RecNr);
};

bool SearchKey(XString& XX, XKey* Key, longint& NN);
longint XNRecs(XKey* K);
void TryInsertAllIndexes(longint RecNr);
void DeleteXRec(longint RecNr, bool DelT);
void OverWrXRec(longint RecNr, void* P2, void* P);
