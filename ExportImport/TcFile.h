#pragma once
#include <cstdint>

#include "../Common/typeDef.h"

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
	TcFile(bool compress);
	virtual ~TcFile();

	size_t iBuf = 0, iBuf2 = 0;
	size_t lBuf = 0, lBuf2 = 0;
	size_t BufSize = 0, BufSize2 = 0;

	uint8_t* buffer1 = nullptr;
	uint8_t* buffer2 = nullptr;

	bool eof = false, eof2 = false;

	bool Compress = false;
	BYTE CodeMask = 0;
	WORD CodeMaskW = 0, lCode = 0, lInput = 0, nToRead = 0, iRingBuf = 0, jRingBuf = 0;
	WORD MatchPos = 0, MatchLen = 0; // set by InsertNode
	BYTE CodeBuf[17]{ 0 }; // [0]=8 flags:"1"=unencoded byte,"0"=position/length word
	TXBuf* XBuf = nullptr;
	int32_t MyDiskFree(bool floppy, char drive_letter);
	void InsertNode(short r);
	void DeleteNode(short p);
	void WriteCodeBuf();
	void InitBufOutp();
	virtual void WriteBuf(bool isLast);
	virtual void WriteBuf2();
	void InitBufInp();
	virtual void ReadBuf();
	virtual void ReadBuf2();
};
