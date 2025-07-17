#pragma once
#include "Fand0File.h"
#include "../Core/switches.h"
#ifdef FandSQL
#include "channel.h"
#endif

extern std::string LockModeTxt[9];
extern const int TransLock;
extern const int ModeLock;
extern const int RecLock;

//void RunErrorM(FileD* file_d, LockMode Md, WORD N);
bool TryLockH(HANDLE Handle, int Pos, uint16_t Len);
bool TryLMode(FileD* fileD, std::string& path, LockMode Mode, LockMode& OldMode, uint16_t Kind, uint16_t lan_node);
void OldLMode(FileD* fileD, std::string& path, LockMode Mode, uint16_t lan_node);
LockMode NewLMode(FileD* fileD, std::string& path, LockMode Mode, uint16_t lan_node);
void UnLockN(Fand0File* fand_file, int N);
bool ChangeLMode(FileD* fileD, std::string& path, LockMode Mode, uint16_t Kind, bool RdPref, uint16_t lan_node);