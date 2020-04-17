#pragma once
#include <set>

#include "access.h"
#include "base.h"
#include "constants.h"

set<char> Handles;
set<char> UpdHandles;
set<char> FlshHandles;

WORD ReadH(WORD handle, WORD bytes, void* buffer);

longint PosH(WORD handle);
longint MoveH(longint dist, WORD method, WORD handle);
WORD OpenH(FileOpenMode Mode, FileUseMode UM);
void SeekH(WORD handle, longint pos);
void CloseH(WORD handle);
void CloseClearH(WORD &h);
bool IsUpdHandle(WORD H);
void RdWrCache(bool ReadOp, WORD Handle, bool NotCached, longint Pos, WORD N, void* Buf);
void DeleteFile(PathStr path);
void TruncH(WORD handle, longint N);
longint FileSizeH(WORD handle);
void WriteH(WORD handle, WORD bytes, void* buffer);
WORD GetFileAttr();
void SetFileAttr(WORD Attr);
