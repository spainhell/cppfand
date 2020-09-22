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

class PressedKey
{
public:
	PressedKey() = default;
	PressedKey(KEY_EVENT_RECORD& key);
	char Char();
	unsigned __int32 KeyDescr();
	unsigned __int32 SimpleKeyDescr();
	bool Shift();
	bool Alt();
	bool Ctrl();
	bool Enhanced();
	bool CapsLock();
	bool ScrollLock();
	bool NumLock();
	bool LeftAlt();
	bool RightAlt();
	bool LeftCtrl();
	bool RightCtrl();
private:
	KEY_EVENT_RECORD _key;
};

