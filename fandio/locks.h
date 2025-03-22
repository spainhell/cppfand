#pragma once
#include "Fand0File.h"
#include "../Core/switches.h"
#ifdef FandSQL
#include "channel.h"
#endif

extern std::string LockModeTxt[9];

//void RunErrorM(FileD* file_d, LockMode Md, WORD N);
bool TryLMode(FileD* fileD, std::string& path, LockMode Mode, LockMode& OldMode, WORD Kind, uint16_t lan_node);
void OldLMode(FileD* fileD, std::string& path, LockMode Mode, uint16_t lan_node);
LockMode NewLMode(FileD* fileD, std::string& path, LockMode Mode, uint16_t lan_node);
bool TryLockN(Fand0File* fand_file, std::string& path, int N, WORD Kind);
void UnLockN(Fand0File* fand_file, int N);
bool ChangeLMode(FileD* fileD, std::string& path, LockMode Mode, WORD Kind, bool RdPref, uint16_t lan_node);