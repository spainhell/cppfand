#include "XXPage.h"

#include "XWFile.h"
#include "XWorkFile.h"
#include "../cppfand/FileD.h"
#include "../cppfand/GlobalVariables.h"
#include "../cppfand/obaseww.h"


void XXPage::Reset(XWorkFile* OwnerXW)
{
	XW = OwnerXW; Sum = 0; NItems = 0;
	memset(A, '\0', sizeof(A));
	Off = 0;
}

void XXPage::PutN(longint N)
{
	memcpy(&A[Off], &N, 3); // kopirujeme 3 nejnizsi Byty, posledni se ignoruje
	Off += 3;
}

void XXPage::PutDownPage(longint DownPage)
{
	memcpy(&A[Off], &DownPage, 4);
	Off += 4;
}

void XXPage::PutMLX(BYTE M, BYTE L)
{
	A[Off++] = M;
	A[Off++] = L;
	memcpy(&A[Off], &LastIndex[M + 1], L);
	Off += L;
}

void XXPage::ClearRest()
{
	// max. offset je 1 mensi nez delka pole
	// aktualni offset vcetne -> maximalni offset vcetne (proto +1)
	//size_t count = XPageSize - Off;
	//memset(&A[Off], 0, sizeof(A) - Off);
}

void XXPage::PageFull()
{
	longint n = 0;
	ClearRest();
	if (Chain == nullptr) {
		Chain = new XXPage();
		Chain->Reset(XW);
	}
	if (IsLeaf) {
		n = XW->nextXPage;
	}
	else {
		n = XW->xwFile->NewPage(XW->xPage);
	}
	Chain->AddToUpper(this, n);
	if (IsLeaf) {
		XW->nextXPage = XW->xwFile->NewPage(XW->xPage);
		GreaterPage = XW->nextXPage;
	}

	XW->xwFile->WrPage(this, n);
}

void XXPage::AddToLeaf(WRec* R, XKey* KD)
{
	BYTE m, l;
	longint n;

	while (true) {
		m = 0;
		l = R->X.S.length();
		n = R->GetN();
		if ((l > 0) && (NItems > 0)) {
			m = SLeadEqu(R->X.S, LastIndex);
			if ((m == l) && (m == LastIndex.length())) {
				if (n == LastRecNr) return; /* overlapping intervals from  key in .. */
				if (!KD->InWork && !KD->Duplic) {
					if (!XW->msgWritten) {
						SetMsgPar(CFile->Name);
						if (IsTestRun) {
							if (!PromptYN(832)) {
								GoExit();
							}
						}
						else {
							WrLLF10Msg(828);
						}
						XW->msgWritten = true;
					}
					ReadRec(CFile, n, CRecPtr);
					for (XKey* K : CFile->Keys) {
						K->Delete(n);
					}
					SetDeletedFlag();
					WriteRec(CFile, n, CRecPtr);
					return;
				}
			}
			l = l - m;
		}
		if (Off + 5 + l > XPageSize - 7) { // 7B je delka hlavicky
			PageFull();
			Reset(XW);
			continue;
		}
		break;
	}
	LastIndex = R->X.S;
	LastRecNr = n;
	Sum++;
	NItems++;
	PutN(n);
	PutMLX(m, l);
}

void XXPage::AddToUpper(XXPage* P, longint DownPage)
{
	WORD l = 0, m = 0;
	while (true) {
		WORD m = 0;
		WORD l = P->LastIndex.length();
		if ((l > 0) && (NItems > 0)) {
			m = SLeadEqu(P->LastIndex, LastIndex);
			l = l - m;
		}
		if (Off + 9 + l > XPageSize - 7) { // 7B je delka hlavicky
			PageFull();
			Reset(XW);
			continue;
		}
		break;
	}
	LastIndex = P->LastIndex;
	Sum += P->Sum;
	NItems++;
	PutN(P->Sum);
	PutDownPage(DownPage);
	PutMLX(m, l);
}
