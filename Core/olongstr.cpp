#include "olongstr.h"

#include "GlobalVariables.h"
#include "oaccess.h"
#include "../fandio/FandTFile.h"

void GetTxtPrepare(FileD* file_d, FrmlElem16* Z, HANDLE* h, int& off, int& len, void* record)
{
	int l = 0;
	off = 0;
	if (Z->P1 != nullptr) {
		off = RunInt(file_d, Z->P1, record) - 1;
		if (off < 0) off = 0;
	}
	SetTxtPathVol(Z->TxtPath, Z->TxtCatIRec);
	TestMountVol(CPath[1]);
	*h = OpenH(CPath, _isOldFile, RdOnly);
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
	if (Z->P2 != nullptr) {
		l = RunInt(file_d, Z->P2, record);
		if (l < 0) l = 0;
		if (l < len) len = l;
	}
	SeekH(*h, off);
}

LongStr* GetTxt(FileD* file_d, FrmlElem16* Z, void* record)
{
	HANDLE h = nullptr;
	int len = 0, off = 0;
	LongStr* s = nullptr;

	GetTxtPrepare(file_d, Z, &h, off, len, record);

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

int CopyTFFromGetTxt(FileD* file_d, FandTFile* TF, FrmlElem16* Z, void* record)
{
	LockMode md;
	int len = 0, off = 0, pos = 0, nxtpos = 0;
	HANDLE h = nullptr;
	WORD n = 0, l = 0, i = 0;
	short rest = 0;
	BYTE X[MPageSize + 1]{ 0 };
	WORD* ll = (WORD*)X;
	bool continued = false;
	int result = 0;

	GetTxtPrepare(file_d, Z, &h, off, len, record);
	LastTxtPos = off + len;
	if (len == 0) {
		result = 0;
		CloseH(&h);
		exit(-1);
	}
	if (!TF->IsWork) md = file_d->NewLockMode(WrMode);
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
			TF->WriteData(TF->FreePart, 2, &rest); //WriteCache(TF, TF->NotCached(), TF->FreePart, 2, &rest);
		}
		TF->WriteData(pos, 2, &l); //WriteCache(TF, TF->NotCached(), pos, 2, &l);
		TF->WriteData(pos + 2, l, X); //WriteCache(TF, TF->NotCached(), pos + 2, l, X);
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
		TF->WriteData(pos, MPageSize, X); //WriteCache(TF, TF->NotCached(), pos, MPageSize, X);
		pos = nxtpos;
		l -= n;
		if (continued && (l == 0)) goto label1;
		goto label3;
	}
	ReadH(h, l, &X[i]);
	TF->WriteData(pos, MPageSize, X); //WriteCache(TF, TF->NotCached(), pos, MPageSize, X);
label4:
	if (!TF->IsWork) file_d->OldLockMode(md);
	CloseH(&h);
	return result;
}
