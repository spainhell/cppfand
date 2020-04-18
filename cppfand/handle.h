#pragma once
#include <set>

#include "access.h"
#include "base.h"
#include "constants.h"

WORD OldNumH; // r1 
void* OldHTPtr = nullptr;
#ifdef FandDemo
WORD files = 30;
#else
WORD files = 250; // {files in CONFIG.SYS -3}
#endif
WORD CardHandles;

set<char> Handles;
set<char> UpdHandles;
set<char> FlshHandles;

void SetRes(WORD FLAGS, WORD AX); // r11 - toto je spec. MSDOS registers, flags
bool IsHandle(WORD H); // r13
bool IsUpdHandle(WORD H); // r15
bool IsFlshHandle(WORD H); // r17
void SetHandle(WORD H);
void SetUpdHandle(WORD H);
void SetFlshHandle(WORD H);
void ResetHandle(WORD H);
void ResetUpdHandle(WORD H);
void ResetFlshHandle(WORD H);
void ClearHandles();
void ClearUpdHandles();
void ClearFlshHandles();
bool IsNetCVol();
void ExtendHandles(); // r55 - práce s pamìtí - pøesun nìkam
void UnExtendHandles();  // -''- - pøesun zpìt

longint MoveH(longint dist, WORD method, WORD handle); // r66 - soubor, nastavení ukazatele (INT $42)
longint PosH(WORD handle); // -''-
void SeekH(WORD handle, longint pos); // -''-
longint FileSizeH(WORD handle); // -''-
bool TryLockH(WORD Handle, longint Pos, WORD Len); // r91 - volá FileLock (INT $5C)
bool UnLockH(WORD Handle, longint Pos, WORD Len); // -''-

WORD OpenH(FileOpenMode Mode, FileUseMode UM); // INT $3D, $3C, $5B
WORD ReadH(WORD handle, WORD bytes, void* buffer); // INT $3F
WORD ReadLongH(WORD handle, longint bytes, void* buffer); // øeší segmenatci a ète postupnì
void WriteH(WORD handle, WORD bytes, void* buffer); // zápis INT $40
void WriteLongH(WORD handle, longint bytes, void* buffer); // -''-
void CloseH(WORD handle); // uzavøení souboru INT $3E
void FlushH(WORD handle); // duplikát handleru INT $45
void FlushHandles();
void TruncH(WORD handle, longint N);
void CloseClearH(WORD& h);
void SetFileAttr(WORD Attr); // INT $4301
WORD GetFileAttr(); // INT $4300
void RdWrCache(bool ReadOp, WORD Handle, bool NotCached, longint Pos, WORD N, void* Buf); // práce s cache


// _____________________________________________________
// od r. 254

longint GetDateTimeH(WORD handle);
void DeleteFile(pstring path); // INT $41
void RenameFile56(pstring OldPath, pstring NewPaht, bool Msg); // INT $56
pstring MyFExpand(pstring Nm, pstring EnvName);
