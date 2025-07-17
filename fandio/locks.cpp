#include "locks.h"

//#include "../Core/GlobalVariables.h"
//#include "../Core/access.h"
#include "../Core/obaseww.h"

//void RunErrorM(FileD* file_d, LockMode Md, WORD N)
//{
//	OldLMode(file_d, Md);
//	RunError(N);
//}

std::string LockModeTxt[9] = { "NULL", "NOEXCL", "NODEL", "NOCR", "RD", "WR", "CR", "DEL", "EXCL" };

#ifdef FandNetV
// const int TransLock = 0x0A000501;  /* locked while state transition */
const int TransLock = 0x40000501; // MB160
// const int ModeLock = 0x0A000000;  /* base for mode locking */
const int ModeLock = 0x40000000;  // MB160
// const int RecLock = 0x0B000000;  /* base for record locking */
const int RecLock = 0x41000000;   // MB160

bool TryLockH(HANDLE Handle, int Pos, uint16_t Len)
{
	OVERLAPPED sOverlapped;
	sOverlapped.Offset = Pos;
	sOverlapped.OffsetHigh = 0;
	auto fSuccess = LockFileEx(Handle, LOCKFILE_EXCLUSIVE_LOCK | LOCKFILE_FAIL_IMMEDIATELY, 0, Len, 0, &sOverlapped);

	if (!fSuccess) {
		LPVOID lpMsgBuf;
		LPVOID lpDisplayBuf;
		DWORD dw = GetLastError();

		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);

		return false;
	}
	return true;
}

bool UnLockH(HANDLE Handle, int Pos, uint16_t Len)
{
	OVERLAPPED sOverlapped;
	sOverlapped.Offset = Pos;
	sOverlapped.OffsetHigh = 0;
	auto fSuccess = UnlockFileEx(Handle, 0, Len, 0, &sOverlapped);

	if (!fSuccess) {
		LPVOID lpMsgBuf;
		LPVOID lpDisplayBuf;
		DWORD dw = GetLastError();

		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);

		return false;
	}
	return true;
}

void ModeLockBnds(LockMode Mode, int& Pos, uint16_t& Len, uint16_t lan_node)
{
	__int32 n = 0;
	switch (Mode) {       /* hi=how much BYTEs, low= first BYTE */
	case NoExclMode: n = 0x00010000 + lan_node; break;
	case NoDelMode: n = 0x00010100 + lan_node; break;
	case NoCrMode: n = 0x00010200 + lan_node; break;
	case RdMode: n = 0x00010300 + lan_node; break;
	case WrMode: n = 0x00FF0300; break;
	case CrMode: n = 0x01FF0200; break;
	case DelMode: n = 0x02FF0100; break;
	case ExclMode: n = 0x03FF0000; break;
	default:;
	}
	Pos = ModeLock + (n & 0xFFFF);
	Len = n >> 16;
}

bool ChangeLMode(FileD* fileD, std::string& path, LockMode Mode, uint16_t Kind, bool RdPref, uint16_t lan_node)
{
	int oldpos; uint16_t oldlen, d;
	bool result = false;
	if (!fileD->FF->IsShared()) {         /*neu!!*/
		result = true;
		fileD->FF->LMode = Mode;
		return result;
	}
	result = false;
	LockMode oldmode = fileD->FF->LMode;
	HANDLE h = fileD->FF->Handle;
	if (oldmode >= WrMode) {
		if (Mode < WrMode) {
			fileD->FF->WrPrefixes();
		}
		if (oldmode == ExclMode) {
			SaveCache(0, fileD->FF->Handle);
			// ClearCacheCFile(); - this method does not exist anymore (we don't use a cache)
		}
		if (Mode < WrMode) {
			fileD->FF->ClearUpdateFlag();
		}
	}
	int w = 0;
	uint16_t count = 0;
label1:
	if (Mode != NullMode)
		if (!TryLockH(h, TransLock, 1)) {
		label2:
			if (Kind == 2) return result; /*0 Kind-wait, 1-wait until ESC, 2-no wait*/
			count++;
			if (count <= spec.LockRetries) {
				d = spec.LockDelay;
			}
			else {
				d = spec.NetDelay;
				fileD->SetPathAndVolume();
				SetMsgPar(path, LockModeTxt[Mode]);
				int w1 = PushWrLLMsg(825, Kind == 1);
				if (w == 0) {
					w = w1;
				}
				else {
					PopW(w1, false);
				}
				LockBeep();
			}
			if (KbdTimer(spec.NetDelay, Kind)) {
				goto label1;
			}
			if (w != 0) PopW(w);
			return result;
		}
	if (oldmode != NullMode) {
		ModeLockBnds(oldmode, oldpos, oldlen, lan_node);
		UnLockH(h, oldpos, oldlen);
	}
	if (Mode != NullMode) {
		uint16_t len;
		__int32 pos;
		ModeLockBnds(Mode, pos, len, lan_node);
		if (!TryLockH(h, pos, len)) {
			if (oldmode != NullMode) {
				TryLockH(h, oldpos, oldlen);
			}
			UnLockH(h, TransLock, 1);
			goto label2;
		}
		UnLockH(h, TransLock, 1);
	}
	if (w != 0) {
		PopW(w);
	}
	fileD->FF->LMode = Mode;
	if ((oldmode < RdMode) && (Mode >= RdMode) && RdPref) {
		int rp = fileD->FF->RdPrefixes();
		if (rp != 0) {
			fileD->CFileError(rp);
		}
	}
	result = true;
	return result;
}

#else
bool ChangeLMode(FileD* fileD, LockMode Mode, uint16_t Kind, bool RdPref)
{
	fileD->LMode = Mode;
	return true;
}
#endif

void OldLMode(FileD* fileD, std::string& path, LockMode Mode, uint16_t lan_node)
{
#ifdef FandSQL
	if (fileD->IsSQLFile) { fileD->LMode = Mode; return; }
#endif
	if (fileD->FF->Handle == nullptr) return;
	if (Mode != fileD->FF->LMode) ChangeLMode(fileD, path, Mode, 0, true, lan_node);
}

bool TryLMode(FileD* fileD, std::string& path, LockMode Mode, LockMode& OldMode, uint16_t Kind, uint16_t lan_node)
{
	bool result = true;
#ifdef FandSQL
	if (fileD->IsSQLFile) {
		OldMode = fileD->LMode; if (Mode > fileD->LMode) fileD->LMode = Mode;
	}
	else
#endif
	{
		if (fileD->FF->Handle == nullptr) {
			fileD->OpenCreateF(path, Shared);
		}
		OldMode = fileD->FF->LMode;
		if (Mode > fileD->FF->LMode) {
			result = ChangeLMode(fileD, path, Mode, Kind, true, lan_node);
		}
	}
	return result;
}

LockMode NewLMode(FileD* fileD, std::string& path, LockMode Mode, uint16_t lan_node)
{
	LockMode md;
	TryLMode(fileD, path, Mode, md, 0, lan_node);
	return md;
}

void UnLockN(Fand0File* fand_file, int N)
{
#ifdef FandSQL
	if (fand_file->GetFileD()->IsSQLFile) return;
#endif
#ifdef FandNetV
	if ((fand_file->Handle == nullptr) || !fand_file->IsShared()) {
		return;
	}
	UnLockH(fand_file->Handle, RecLock + N, 1);
#endif
	}
