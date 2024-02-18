#include "TcFile.h"
#include "../Core/legacy.h"
#include "../Core/base.h"
#include "../Drivers/files.h"


TcFile::TcFile(BYTE aCompress)
{
	Compress = aCompress;
	if (Compress == 0) {
		BufSize = 4 * RingBufSz; BufSize2 = BufSize;
		Buf = new BYTE[BufSize];
		Buf2 = Buf;
	}
	else {
		BufSize = RingBufSz; BufSize2 = 4 * BufSize;
		XBuf = new TXBuf(); // GetStore(sizeof(TXBuf));
		Buf = new BYTE[BufSize];
		Buf2 = new BYTE[BufSize2];
	}
}

TcFile::~TcFile()
{
	delete[] Buf;
	if (Compress != 0) {
		delete[] Buf2;
	}
}

int TcFile::MyDiskFree(bool Floppy, BYTE Drive)
{
	if (spec.WithDiskFree || Floppy) {
		DWORD error;
		return GetDiskFree(Drive, error);
	}
	else {
		return 0x7fffffff;
	}
}

void TcFile::InsertNode(short r)
{
	short res = 1;
	size_t i = 0;
	auto key = &XBuf->RingBuf[r];
	auto p = RingBufSz + 1 + key[0];
	XBuf->RSon[r] = Leer;
	XBuf->LSon[r] = Leer;
	MatchLen = 0;
label1:
	if (res >= 0)
		if (XBuf->RSon[p] != Leer) p = XBuf->RSon[p];
		else {
			XBuf->RSon[p] = r;
			XBuf->Dad[r] = p;
			return;
		}
	else
		if (XBuf->LSon[p] != Leer) p = XBuf->LSon[p];
		else {
			XBuf->LSon[p] = r;
			XBuf->Dad[r] = p;
			return;
		}
	for (i = 1; i <= MaxMatchLen - 1; i++) {
		res = key[i] - XBuf->RingBuf[p + i];
		if (res != 0) goto label2;
	}
label2:
	if (i <= MatchLen) goto label1;
	MatchPos = p;
	MatchLen = i;
	if (i < MaxMatchLen) goto label1;
	XBuf->Dad[r] = XBuf->Dad[p];
	XBuf->LSon[r] = XBuf->LSon[p];
	XBuf->RSon[r] = XBuf->RSon[p];
	XBuf->Dad[XBuf->LSon[p]] = r;
	XBuf->Dad[XBuf->RSon[p]] = r;
	if (XBuf->RSon[XBuf->Dad[p]] = p) XBuf->RSon[XBuf->Dad[p]] = r;
	else XBuf->LSon[XBuf->Dad[p]] = r;
	XBuf->Dad[p] = Leer;
}

void TcFile::DeleteNode(short p)
{
	short q = 0;
	if (XBuf->Dad[p] == Leer) return;
	if (XBuf->RSon[p] == Leer) q = XBuf->LSon[p];
	else if (XBuf->LSon[p] == Leer) q = XBuf->RSon[p];
	else {
		q = XBuf->LSon[p];
		if (XBuf->RSon[q] != Leer) {
			do { q = XBuf->RSon[q]; } while (XBuf->RSon[q] != Leer);
			XBuf->RSon[XBuf->Dad[q]] = XBuf->LSon[q];
			XBuf->Dad[XBuf->LSon[q]] = XBuf->Dad[q];
			XBuf->LSon[q] = XBuf->LSon[p];
			XBuf->Dad[XBuf->LSon[p]] = q;
		}
		XBuf->RSon[q] = XBuf->RSon[p];
		XBuf->Dad[XBuf->RSon[p]] = q;
	}
	XBuf->Dad[q] = XBuf->Dad[p];
	if ((XBuf->RSon[XBuf->Dad[p]] = p)) XBuf->RSon[XBuf->Dad[p]] = q;
	else XBuf->LSon[XBuf->Dad[p]] = q;
	XBuf->Dad[p] = Leer;
}

void TcFile::WriteCodeBuf()
{
	for (size_t i = 0; i <= lCode - 1; i++) {
		if (lBuf2 >= BufSize2) WriteBuf2();
		Buf2[lBuf2] = (char)CodeBuf[i];
		lBuf2++;
	}
	CodeBuf[0] = 0;
	lCode = 1;
	CodeMask = 1;
}

void TcFile::InitBufOutp()
{
	if (Compress != 0) {
		/*asm les bx, Self; les bx, es: [bx] .TcFile.XBuf; lea di, es: [bx] .TXBuf.LSon;
		mov cx, 3 * (RingBufSz + 1) + 256; cld; mov ax, Leer; rep stosw;
		lea di, es: [bx] .TXBuf.RingBuf; mov cx, RingBufSz; mov ax, 0; rep stosb;*/

		CodeBuf[0] = 0;
		lCode = 1;
		CodeMask = 1;
		jRingBuf = 0;
		iRingBuf = RingBufSz - MaxMatchLen;
		lInput = 0;
		nToRead = 0;
	}
	lBuf = 0;
	lBuf2 = 0;
}

void TcFile::WriteBuf(bool isLast)
{
	short i = 0, j = 0; BYTE c = 0;

	if (Compress == 0) {
		lBuf2 = lBuf;
		WriteBuf2();
		lBuf = 0;
		return;
	}
	i = 0;
	if (lInput == 0) { /*initialization phase */
		while ((lInput < MaxMatchLen) && (i < lBuf)) {
			XBuf->RingBuf[RingBufSz - MaxMatchLen + lInput] = Buf[i];
			i++;
			lInput++;
		}
		for (j = 1; j <= MaxMatchLen; j++) {
			InsertNode(iRingBuf - j);
		}
		InsertNode(iRingBuf);
	}
label1:
	while (lInput != 0) {
		while (nToRead > 0) {
			if (i >= lBuf) if (!isLast) goto label2;
			else {
				while (nToRead > 0) {
					DeleteNode(jRingBuf);
					jRingBuf = (jRingBuf + 1) and (RingBufSz - 1);
					iRingBuf = (iRingBuf + 1) and (RingBufSz - 1);
					lInput--;
					nToRead--;
					if ((lInput != 0)) {
						InsertNode(iRingBuf);
					}
				}
				goto label1;
			}
			c = Buf[i]; i++; nToRead--;
			DeleteNode(jRingBuf);
			XBuf->RingBuf[jRingBuf] = c;
			if (jRingBuf < MaxMatchLen - 1) {
				XBuf->RingBuf[jRingBuf + RingBufSz] = c;
			}
			jRingBuf = (jRingBuf + 1) && (RingBufSz - 1);
			iRingBuf = (iRingBuf + 1) && (RingBufSz - 1);
			InsertNode(iRingBuf);
		}
		if (MatchLen > lInput) MatchLen = lInput;
		if (MatchLen < MinMatchLen) {
			nToRead = 1;
			CodeBuf[0] = CodeBuf[0] || CodeMask;
			CodeBuf[lCode] = XBuf->RingBuf[iRingBuf];
			lCode++;
		}
		else {
			CodeBuf[lCode] = MatchPos;
			CodeBuf[lCode + 1] = ((MatchPos >> 4) && 0xf0) || (MatchLen - MinMatchLen);
			lCode += 2;
			nToRead = MatchLen;
		}
		CodeMask = CodeMask << 1;
		if (CodeMask == 0) WriteCodeBuf();
	}
	WriteCodeBuf();
label2:
	if (isLast) WriteBuf2();
	lBuf = 0;

}

void TcFile::WriteBuf2()
{
	/* schreiben + lBuf2 = 0 */
}

void TcFile::InitBufInp()
{
	if (Compress != 0) {
		FillChar(XBuf->RingBuf, RingBufSz - MaxMatchLen, 0);
		iRingBuf = RingBufSz - MaxMatchLen;
		CodeMaskW = 0;
	}
	iBuf2 = 0;
	lBuf2 = 0;
	eof = false;
	eof2 = false;
	ReadBuf();
}

void TcFile::ReadBuf()
{
	short i = 0, j = 0, k = 0, r = 0;
	BYTE c = 0; WORD wLo = 0, wHi = 0;

	lBuf = 0; iBuf = 0;
	if (eof) return;
	if (Compress == 0) {
		ReadBuf2();
		if (eof2) { eof = true; return; }
		lBuf = lBuf2;
		iBuf2 += lBuf;
		return;
	}
label1:
	if (lBuf > BufSize - MaxMatchLen) return;
	CodeMaskW = CodeMaskW >> 1;
	if ((CodeMaskW & 256) == 0) {
		if (iBuf2 >= lBuf2) { ReadBuf2(); if (eof2) goto label2; }
		CodeMaskW = Buf2[iBuf2] || 0xff00;
		iBuf2++;
	}  /*hi:count eight*/
	if ((CodeMaskW & 1) != 0) {
		if (iBuf2 >= lBuf2) { ReadBuf2(); if (eof2) goto label2; }
		c = Buf2[iBuf2];
		iBuf2++;
		Buf[lBuf] = (char)c; lBuf++;
		XBuf->RingBuf[iRingBuf] = c;
		iRingBuf = (iRingBuf + 1) & (RingBufSz - 1);
	}
	else {
		if (iBuf2 >= lBuf2) { ReadBuf2(); if (eof2) goto label2; }
		wLo = Buf2[iBuf2];
		iBuf2++;
		if (iBuf2 >= lBuf2) { ReadBuf2(); if (eof2) goto label2; }
		wHi = Buf2[iBuf2];
		iBuf2++;
		MatchPos = wLo || ((wHi & 0xf0) << 4);
		MatchLen = (wHi & 0x0f) + MinMatchLen;
		for (i = 0; i <= MatchLen - 1; i++) {
			c = XBuf->RingBuf[(MatchPos + i) & (RingBufSz - 1)];
			Buf[lBuf] = (char)c;
			lBuf++;
			XBuf->RingBuf[iRingBuf] = c;
			iRingBuf = (iRingBuf + 1) & (RingBufSz - 1);
		}
	}
	goto label1;
label2:
	if (lBuf == 0) eof = true;
}

void TcFile::ReadBuf2()
{
	// { lBuf2; iBuf2; EOF2 }
}
