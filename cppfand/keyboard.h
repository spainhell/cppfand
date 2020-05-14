#pragma once
#include <windows.h>

class Keyboard
{
public:
	Keyboard();
	~Keyboard();
	bool Exists();
	bool Empty();
	size_t BufSize();
	void ClearBuf();
	size_t ActualIndex();
	size_t FreeSpace();
	bool Get(KEY_EVENT_RECORD& key);
	
private:
	HANDLE _handle;
	PINPUT_RECORD _kbdBuf;
	size_t _actualIndex;
	DWORD _inBuffer;
	void _read();
};

