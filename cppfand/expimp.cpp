#include "expimp.h"
#include "oaccess.h"

enum InOutMode { _inp, _outp, _append };

const WORD RingBufSz = 4096;
const BYTE MaxMatchLen = 18;
const BYTE MinMatchLen = 3;
const WORD Leer = RingBufSz;

struct TXBuf {
	BYTE RingBuf[RingBufSz + MaxMatchLen - 1]{ 0 };
	WORD LSon[RingBufSz + 1]{ 0 };			// {binary search trees; son/dad[iNode]}
	WORD Dad[RingBufSz + 1]{ 0 };			// {binary search trees; son/dad[iNode]}
	WORD RSon[RingBufSz + 256 + 1]{ 0 };	// {last 256: root for strings beginning with c}
};

class TcFile {
public:
	TcFile(BYTE aCompress);
	~TcFile();
	char* Buf = nullptr;
	char* Buf2 = nullptr;
	WORD iBuf = 0, lBuf = 0, iBuf2 = 0, lBuf2 = 0, BufSize = 0, BufSize2 = 0;
	bool eof = false, eof2 = false;
	BYTE Compress = 0; BYTE CodeMask = 0;
	WORD CodeMaskW = 0, lCode = 0, lInput = 0, nToRead = 0, iRingBuf = 0, jRingBuf = 0;
	WORD MatchPos = 0, MatchLen = 0; // set by InsertNode
	BYTE CodeBuf[17]{ 0 }; // [0]=8 flags:"1"=unencoded byte,"0"=position/length word
	TXBuf* XBuf = nullptr;
	longint MyDiskFree(bool Floppy, BYTE Drive);
	void InsertNode(integer r);
	void DeleteNode(integer p);
	void WriteCodeBuf();
	void InitBufOutp();
	void WriteBuf(bool isLast);
	virtual void WriteBuf2();
	void InitBufInp();
	void ReadBuf();
	virtual void ReadBuf2();
};

class ThFile : public TcFile
{
public:
	ThFile(std::string APath, WORD CatIRec, InOutMode AMode, byte aCompress, ThFile* F);
	~ThFile();
	FILE* Handle = nullptr;
	std::string Path;
	std::string Vol;
	InOutMode Mode = _inp;
	bool Floppy = false, IsEOL = false, Continued = false;
	longint Size = 0, OrigSize = 0, SpaceOnDisk = 0;
	FileD* FD = nullptr;
	void Append();
	void ClearBuf();
	void Delete();
	char ForwChar();
	char RdChar();
	std::string RdDM(char Delim, integer Max);
	std::string RdDelim(char Delim);
	std::string RdFix(integer N);
	LongStr* RdLongStr();
	virtual void ReadBuf2();
	void Reset();
	void ExtToT();
	void ResetT();
	void ResetX();
	void Rewrite();
	void RewriteT();
	void RewriteX();
	void TestError();
	bool TestErr152();
	void WrChar(char C);
	virtual void WriteBuf2();
	void WrString(std::string S);
	void WrLongStr(LongStr* S, bool WithDelim);
};





bool OldToNewCat(longint& FilSz)
{
	struct stX { longint NRecs; WORD  RecLen; } x;
	longint off, offNew, i;
	BYTE a[91]; // budeme èíslovat od 1, jako v Pascalu (a:array[1..90] of byte;)
	/* !!! with CFile^ do!!! */
	auto result = false;
	if (CFile->Typ != 'C') return result;
	RdWrCache(true, CFile->Handle, CFile->NotCached(), 0, 6, &x);
	if (x.RecLen != 106) return result;
	x.RecLen = 107;
	RdWrCache(false, CFile->Handle, CFile->NotCached(), 0, 6, &x);
	for (i = x.NRecs; i <= 1; i--) {
		off = 6 + (i - 1) * 106;
		offNew = off + (i - 1);
		RdWrCache(true, CFile->Handle, CFile->NotCached(), off + 16, 90, a);
		RdWrCache(false, CFile->Handle, CFile->NotCached(), offNew + 17, 90, a);
		a[17] = 0;
		RdWrCache(true, CFile->Handle, CFile->NotCached(), off, 16, a);
		RdWrCache(false, CFile->Handle, CFile->NotCached(), offNew, 17, a);
	}
	CFile->NRecs = x.NRecs;
	FilSz = x.NRecs * 107 + 6;
	result = true;
	return result;
}

void CheckFile(FileD* FD)
{
	struct stPrfx { longint NRecs; WORD RecLen; } Prfx;
	FILE* h = nullptr;
	pstring d; pstring n; pstring e;
	longint fs = 0;

	TestMountVol(CPath[1]);
	h = OpenH(_isoldfile, RdShared);
	LastExitCode = 0;
	if (HandleError != 0) {
		if (HandleError == 2) LastExitCode = 1;
		else LastExitCode = 2;
		return;
	}
	ReadH(h, 4, &Prfx.NRecs);
	ReadH(h, 2, &Prfx.RecLen);
	fs = FileSizeH(h);
	CloseH(h);
	if ((FD->RecLen != Prfx.RecLen) || (Prfx.NRecs < 0) && (FD->Typ != 'X') ||
		((fs - FD->FrstDispl) / Prfx.RecLen < Prfx.NRecs) ||
		(Prfx.NRecs > 0) && (FD->Typ == 'X')) {
		LastExitCode = 3;
		return;
	}
	if (FD->TF == nullptr) return;
	FSplit(CPath, d, n, e);
	if (SEquUpcase(e, ".RDB")) e = ".TTT";
	else e[2] = 'T';
	CPath = d + n + e;
	h = OpenH(_isoldfile, RdShared);
	if (HandleError == 0) CloseH(h);
	else LastExitCode = 4;
}

TcFile::TcFile(BYTE aCompress)
{
	Compress = aCompress;
	if (aCompress == 0) {
		BufSize = 4 * RingBufSz; BufSize2 = BufSize;
		Buf = new char[BufSize];
		Buf2 = Buf;
	}
	else {
		BufSize = RingBufSz; BufSize2 = 4 * BufSize;
		XBuf = new TXBuf(); // GetStore(sizeof(TXBuf));
		Buf = new char[BufSize];
		Buf2 = new char[BufSize2];
	}
}

TcFile::~TcFile()
{
	delete[] Buf; delete[] Buf2;
}

longint TcFile::MyDiskFree(bool Floppy, BYTE Drive)
{
	if (spec.WithDiskFree || Floppy) return DiskFree(Drive);
	return 0x7fffffff;
}

void TcFile::InsertNode(integer r)
{
	integer res = 1;
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

void TcFile::DeleteNode(integer p)
{
	integer q = 0;
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
