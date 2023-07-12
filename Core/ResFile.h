#pragma once


#include <string>
#include <vector>

#include "../Common/LongStr.h"
#include "../Drivers/files.h"

const BYTE FandFace = 17;

struct TMsgIdxItem {
	WORD Nr;
	WORD Ofs;
	BYTE Count;
};

struct st {
	int Pos = 0;
	WORD Size = 0;
};

class ResFile
{
public:
	ResFile();
	~ResFile();
	std::string FullName;
	HANDLE Handle = nullptr;
	st A[FandFace];

	void Open(std::string path);
	WORD Get(WORD Kod, void** P);
	std::string Get(WORD Kod);
	LongStr* GetStr(WORD Kod);
	bool ReadMessage(int msg_nr, std::string& message);
	void ReadInfo();

private:
	DWORD error = 0;
	WORD MsgIdxN = 0;
	std::vector<TMsgIdxItem> MsgIdx;
	int FrstMsgPos = 0;
	std::string get_message(int offset, WORD order);
};