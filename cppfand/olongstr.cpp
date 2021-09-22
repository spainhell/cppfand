#include "olongstr.h"

#include "GlobalVariables.h"
#include "oaccess.h"
#include "TFile.h"

void GetTxtPrepare(FrmlElem* Z, FILE** h, longint& off, longint& len)
{
	auto iZ = (FrmlElem16*)Z;
	longint l = 0;
	off = 0;
	if (iZ->PPPPPP1 != nullptr) { 
		off = RunInt(iZ->PPPPPP1) - 1; 
		if (off < 0) off = 0; 
	}
	SetTxtPathVol(iZ->TxtPath, iZ->TxtCatIRec);
	TestMountVol(CPath[1]);
	*h = OpenH(_isoldfile, RdOnly);
	if (HandleError != 0) {
		if (HandleError == 2) {
			*h = nullptr; 
			len = 0; 
			return; 
		}
		TestCPathError();
	}
	len = FileSizeH(*h);
	LastExitCode = 0;
	if (off >= len) off = len;
	len -= off;
	if (iZ->PPPP2 != nullptr) {
		l = RunInt(iZ->PPPP2);
		if (l < 0) l = 0;
		if (l < len) len = l;
	}
	SeekH(*h, off);
}

LongStr* GetTxt(FrmlPtr Z)
{
	FILE* h = nullptr;
	longint len = 0, off = 0;
	LongStr* s = nullptr;

	GetTxtPrepare(Z, &h, off, len);

	if (len > MaxLStrLen) {
		len = MaxLStrLen; 
		LastExitCode = 1;
	}
	s = new LongStr(len);
	LastTxtPos = off + len;
	if (len > 0) ReadH(h, len, s->A);
	s->LL = len;
	CloseH(&h);
	return s;
}

longint CopyTFFromGetTxt(TFile* TF, FrmlElem* Z)
{
	LockMode md; 
	longint len = 0, off = 0, pos = 0, nxtpos = 0; 
	FILE* h = nullptr;
	WORD n = 0, l = 0, i = 0; 
	integer rest = 0;
	BYTE X[MPageSize + 1]{ 0 };
	WORD* ll = (WORD*)X; 
	bool continued = false;
	longint result = 0;

	GetTxtPrepare(Z, &h, off, len);
	LastTxtPos = off + len;
	if (len == 0) {
		result = 0;
		CloseH(&h);
		exit;
	}
	if (!TF->IsWork) md = NewLMode(WrMode);
	if (len <= MPageSize - 2) { /* short text */
		l = (WORD)len;
		ReadH(h, l, X);
		rest = MPageSize - TF->FreePart % MPageSize;
		if (l + 2 <= rest) pos = TF->FreePart;
		else {
			pos = TF->NewPage(false);
			TF->FreePart = pos;
			rest = MPageSize;
		}
		if (l + 4 >= rest) TF->FreePart = TF->NewPage(false);
		else {
			TF->FreePart += l + 2; 
			rest = l + 4 - rest;
			RdWrCache(false, TF->Handle, TF->NotCached(), TF->FreePart, 2, &rest);
		}
		RdWrCache(false, TF->Handle, TF->NotCached(), pos, 2, &l);
		RdWrCache(false, TF->Handle, TF->NotCached(), pos + 2, l, X);
		result = pos;
		goto label4;
	}
	pos = TF->NewPage(false);
	result = pos;
label1:
	if (len > MaxLStrLen) {
		l = MaxLStrLen;
		*ll = l + 1;
		len = len - l;
		continued = true;
	}
	else {
		l = WORD(len);
		*ll = l;
		len = 0;
		continued = false;
	}
	i = 2;
label3:
	if ((l > MPageSize - i) || continued) {
		n = MPageSize - 4 - i;
		if (n > l) n = l;
		ReadH(h, n, &X[i]);
		i = 0;
		nxtpos = TF->NewPage(false);
		*(longint*)X[MPageSize - 4] = nxtpos;
		RdWrCache(false, TF->Handle, TF->NotCached(), pos, MPageSize, X);
		pos = nxtpos;
		l -= n;
		if (continued && (l == 0)) goto label1;
		goto label3;
	}
	ReadH(h, l, &X[i]);
	RdWrCache(false, TF->Handle, TF->NotCached(), pos, MPageSize, X);
label4:
	if (!TF->IsWork) OldLMode(md); 
	CloseH(&h);
	return result;
}

longint CopyTFString(TFile* TF, FileD* FD2, TFile* TF2, longint Pos2)
{
	FileD* cf = nullptr;
	WORD l = 0; integer rest = 0; bool isLongTxt = false, frst = false;
	longint pos = 0, nxtpos = 0; LockMode md, md2;
	BYTE X[MPageSize + 1]{ 0 };
	WORD* ll = (WORD*)X;
	longint result = 0;

	if (Pos2 == 0) {
	label0:
		return 0; /*Mark****/
	}
	cf = CFile;
	if (!TF->IsWork) md = NewLMode(WrMode);
	CFile = FD2;
	if (!TF2->IsWork) md2 = NewLMode(RdMode);
	RdWrCache(true, TF2->Handle, TF2->NotCached(), Pos2, 2, &l);
	if (l <= MPageSize - 2) { /* short text */
		if (l == 0) goto label0; /*Mark****/
		RdWrCache(true, TF2->Handle, TF2->NotCached(), Pos2 + 2, l, X);
		CFile = cf;
		rest = MPageSize - TF->FreePart % MPageSize;
		if (l + 2 <= rest) pos = TF->FreePart;
		else {
			pos = TF->NewPage(false);
			TF->FreePart = pos;
			rest = MPageSize;
		}
		if (l + 4 >= rest) TF->FreePart = TF->NewPage(false);
		else {
			TF->FreePart += l + 2;
			rest = l + 4 - rest;
			RdWrCache(false, TF->Handle, TF->NotCached(), TF->FreePart, 2, &rest);
		}
		RdWrCache(false, TF->Handle, TF->NotCached(), pos, 2, &l);
		RdWrCache(false, TF->Handle, TF->NotCached(), pos + 2, l, X);
		result = pos;
		goto label4;
	}
	if ((Pos2 % MPageSize) != 0) goto label2;
	RdWrCache(true, TF2->Handle, TF2->NotCached(), Pos2, MPageSize, X);
	frst = true;
label1:
	if (l > MaxLStrLen + 1) {
	label2:
		TF2->Err(889, false);
		result = 0;
		goto label4;
	}
	isLongTxt = (l == MaxLStrLen + 1);
	if (isLongTxt) l--;
	l += 2;
label3:
	CFile = cf;
	if (frst) {
		pos = TF->NewPage(false);
		result = pos;
		frst = false;
	}
	if ((l > MPageSize) || isLongTxt) {
		Pos2 = *(longint*)&X[MPageSize - 4];
		nxtpos = TF->NewPage(false);
		*(longint*)&X[MPageSize - 4] = nxtpos;
		RdWrCache(false, TF->Handle, TF->NotCached(), pos, MPageSize, X);
		pos = nxtpos;
		CFile = FD2;
		if ((Pos2 < MPageSize) || (Pos2 + MPageSize > TF2->MLen) || (Pos2 % MPageSize != 0)) {
			TF2->Err(888, false);
			result = 0;
			goto label4;
		}
		RdWrCache(true, TF2->Handle, TF2->NotCached(), Pos2, MPageSize, X);
		if ((l <= MPageSize)) {
			l = *ll;
			goto label1;
		}
		l -= MPageSize - 4;
		goto label3;
	}
	RdWrCache(false, TF->Handle, TF->NotCached(), pos, MPageSize, X);
label4:
	CFile = FD2;
	if (!TF2->IsWork) OldLMode(md2);
	CFile = cf;
	if (!TF->IsWork) OldLMode(md);
	return result;
}

void CopyTFStringToH(FILE* h)
{
	FileD* cf = nullptr; TFilePtr tf = nullptr;
	WORD i = 0, l = 0, n = 0; bool isLongTxt = false; longint pos = 0;
	BYTE X[MPageSize + 1]{ 0 };
	WORD* ll = (WORD*)X;
	LockMode md2;

	pos = TF02Pos;
	if (pos == 0) return;
	cf = CFile;
	CFile = TFD02;
	tf = TF02;
	if (!tf->IsWork) md2 = NewLMode(RdMode);
	RdWrCache(true, tf->Handle, tf->NotCached(), pos, 2, &l);
	if (l <= MPageSize - 2) { /* short text */
		RdWrCache(true, tf->Handle, tf->NotCached(), pos + 2, l, X);
		WriteH(h, l, X);
		goto label4;
	}
	if ((pos % MPageSize) != 0) goto label2;
	RdWrCache(true, tf->Handle, tf->NotCached(), pos, MPageSize, X);
label1:
	if (l > MaxLStrLen + 1) {
	label2:
		tf->Err(889, false);
		goto label4;
	}
	isLongTxt = (l == MaxLStrLen + 1);
	if (isLongTxt) l--;
	i = 2;
label3:
	if ((l > MPageSize - i) || isLongTxt) {
		n = MPageSize - 4 - i;
		if (n > l) n = l;
		WriteH(h, n, &X[i]);
		pos = *(longint*)&X[MPageSize - 4];
		if ((pos < MPageSize) || (pos + MPageSize > tf->MLen) || (pos % MPageSize != 0)) {
			tf->Err(888, false);
			goto label4;
		}
		RdWrCache(true, tf->Handle, tf->NotCached(), pos, MPageSize, X);
		if ((l <= MPageSize - i)) {
			l = *ll;
			goto label1;
		}
		l -= n;
		i = 0;
		goto label3;
	}
	WriteH(h, l, &X[i]);
label4:
	if (!tf->IsWork) OldLMode(md2);
	CFile = cf;
}
