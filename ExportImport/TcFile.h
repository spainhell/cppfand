#pragma once
#include "../cppfand/constants.h"

const size_t BUFFER_SIZE = 16384;

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
	virtual ~TcFile();
	size_t lBuf = BUFFER_SIZE;
	BYTE* Buf = new BYTE[BUFFER_SIZE]{ '\0' };
	BYTE* Buf2 = nullptr;
	WORD iBuf = 0, iBuf2 = 0, lBuf2 = 0, BufSize = 0, BufSize2 = 0;
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
	virtual void WriteBuf(bool isLast);
	virtual void WriteBuf2();
	void InitBufInp();
	virtual void ReadBuf();
	virtual void ReadBuf2();
};
