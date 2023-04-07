#include "olongstr.h"

#include "GlobalVariables.h"
#include "oaccess.h"
#include "../fandio/FandTFile.h"

void GetTxtPrepare(FrmlElem* Z, FILE** h, int& off, int& len)
{
	auto iZ = (FrmlElem16*)Z;
	int l = 0;
	off = 0;
	if (iZ->PPPPPP1 != nullptr) { 
		off = RunInt(iZ->PPPPPP1) - 1; 
		if (off < 0) off = 0; 
	}
	SetTxtPathVol(iZ->TxtPath, iZ->TxtCatIRec);
	TestMountVol(CPath[1]);
	*h = OpenH(CPath, _isoldfile, RdOnly);
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

LongStr* GetTxt(FrmlElem* Z)
{
	FILE* h = nullptr;
	int len = 0, off = 0;
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

int CopyTFFromGetTxt(FandTFile* TF, FrmlElem* Z)
{
	LockMode md; 
	int len = 0, off = 0, pos = 0, nxtpos = 0; 
	FILE* h = nullptr;
	WORD n = 0, l = 0, i = 0; 
	short rest = 0;
	BYTE X[MPageSize + 1]{ 0 };
	WORD* ll = (WORD*)X; 
	bool continued = false;
	int result = 0;

	GetTxtPrepare(Z, &h, off, len);
	LastTxtPos = off + len;
	if (len == 0) {
		result = 0;
		CloseH(&h);
		exit;
	}
	if (!TF->IsWork) md = NewLMode(CFile, WrMode);
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
			RdWrCache(WRITE, TF->Handle, TF->NotCached(), TF->FreePart, 2, &rest);
		}
		RdWrCache(WRITE, TF->Handle, TF->NotCached(), pos, 2, &l);
		RdWrCache(WRITE, TF->Handle, TF->NotCached(), pos + 2, l, X);
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
		*(int*)&X[MPageSize - 4] = nxtpos;
		RdWrCache(WRITE, TF->Handle, TF->NotCached(), pos, MPageSize, X);
		pos = nxtpos;
		l -= n;
		if (continued && (l == 0)) goto label1;
		goto label3;
	}
	ReadH(h, l, &X[i]);
	RdWrCache(WRITE, TF->Handle, TF->NotCached(), pos, MPageSize, X);
label4:
	if (!TF->IsWork) OldLMode(CFile, md);
	CloseH(&h);
	return result;
}

int CopyTFString(FandTFile* destT00File, FileD* srcFileDescr, FandTFile* scrT00File, int srcT00Pos)
{
	//if (destT00File == scrT00File) {
	//	throw std::exception("CopyTFString() exception: Source and destination file is same.");
	//}
	FileD* cf = nullptr;
	WORD l = 0;
	short rest = 0;
	bool isLongTxt = false, frst = false;
	int pos = 0, nxtpos = 0;
	LockMode md, md2;
	BYTE X[MPageSize + 1]{ 0 };
	WORD* ll = (WORD*)X;
	int result = 0;

	if (srcT00Pos == 0) {
	label0:
		return 0; /*Mark****/
	}
	cf = CFile;
	if (!destT00File->IsWork) md = NewLMode(CFile, WrMode);
	CFile = srcFileDescr;
	if (!scrT00File->IsWork) md2 = NewLMode(CFile, RdMode);
	RdWrCache(READ, scrT00File->Handle, scrT00File->NotCached(), srcT00Pos, 2, &l);
	if (l <= MPageSize - 2) { /* short text */
		if (l == 0) goto label0; /*Mark****/
		RdWrCache(READ, scrT00File->Handle, scrT00File->NotCached(), srcT00Pos + 2, l, X);
		CFile = cf;
		rest = MPageSize - destT00File->FreePart % MPageSize;
		if (l + 2 <= rest) pos = destT00File->FreePart;
		else {
			pos = destT00File->NewPage(false);
			destT00File->FreePart = pos;
			rest = MPageSize;
		}
		if (l + 4 >= rest) destT00File->FreePart = destT00File->NewPage(false);
		else {
			destT00File->FreePart += l + 2;
			rest = l + 4 - rest;
			RdWrCache(WRITE, destT00File->Handle, destT00File->NotCached(), destT00File->FreePart, 2, &rest);
		}
		RdWrCache(WRITE, destT00File->Handle, destT00File->NotCached(), pos, 2, &l);
		RdWrCache(WRITE, destT00File->Handle, destT00File->NotCached(), pos + 2, l, X);
		result = pos;
		goto label4;
	}
	if ((srcT00Pos % MPageSize) != 0) {
		goto label2;
	}
	RdWrCache(READ, scrT00File->Handle, scrT00File->NotCached(), srcT00Pos, MPageSize, X);
	frst = true;
label1:
	if (l > MaxLStrLen + 1) {
	label2:
		scrT00File->Err(889, false);
		result = 0;
		goto label4;
	}
	isLongTxt = (l == MaxLStrLen + 1);
	if (isLongTxt) l--;
	l += 2;
label3:
	CFile = cf;
	if (frst) {
		pos = destT00File->NewPage(false);
		result = pos;
		frst = false;
	}
	if ((l > MPageSize) || isLongTxt) {
		srcT00Pos = *(int*)&X[MPageSize - 4];
		nxtpos = destT00File->NewPage(false);
		*(int*)&X[MPageSize - 4] = nxtpos;
		RdWrCache(WRITE, destT00File->Handle, destT00File->NotCached(), pos, MPageSize, X);
		pos = nxtpos;
		CFile = srcFileDescr;
		if ((srcT00Pos < MPageSize) || (srcT00Pos + MPageSize > scrT00File->MLen) || (srcT00Pos % MPageSize != 0)) {
			scrT00File->Err(888, false);
			result = 0;
			goto label4;
		}
		RdWrCache(READ, scrT00File->Handle, scrT00File->NotCached(), srcT00Pos, MPageSize, X);
		if ((l <= MPageSize)) {
			l = *ll;
			goto label1;
		}
		l -= MPageSize - 4;
		goto label3;
	}
	RdWrCache(WRITE, destT00File->Handle, destT00File->NotCached(), pos, MPageSize, X);
label4:
	CFile = srcFileDescr;
	if (!scrT00File->IsWork) OldLMode(CFile, md2);
	CFile = cf;
	if (!destT00File->IsWork) OldLMode(CFile, md);
	return result;
}

void CopyTFStringToH(FILE* h, FandTFile* TF02, FileD* TFD02, int& TF02Pos)
{
	WORD i = 0;
	bool isLongTxt = false;
	int pos = 0;
	size_t n = 0;
	BYTE X[MPageSize + 1]{ 0 };
	WORD* ll = (WORD*)X;
	LockMode md2;

	pos = TF02Pos;
	if (pos == 0) return;
	FileD* cf = CFile;
	CFile = TFD02;
	TFilePtr tf = TF02;
	if (!tf->IsWork) md2 = NewLMode(CFile, RdMode);
	size_t l = 0;
	RdWrCache(READ, tf->Handle, tf->NotCached(), pos, 2, &l);
	if (l <= MPageSize - 2) { /* short text */
		RdWrCache(READ, tf->Handle, tf->NotCached(), pos + 2, l, X);
		WriteH(h, l, X);
		goto label4;
	}
	if ((pos % MPageSize) != 0) goto label2;
	RdWrCache(READ, tf->Handle, tf->NotCached(), pos, MPageSize, X);
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
		pos = *(int*)&X[MPageSize - 4];
		if ((pos < MPageSize) || (pos + MPageSize > tf->MLen) || (pos % MPageSize != 0)) {
			tf->Err(888, false);
			goto label4;
		}
		RdWrCache(READ, tf->Handle, tf->NotCached(), pos, MPageSize, X);
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
	if (!tf->IsWork) OldLMode(CFile, md2);
	CFile = cf;
}
