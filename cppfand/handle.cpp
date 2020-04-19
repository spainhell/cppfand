#pragma once

#include "handle.h"


#include "windows.h"
#include <fileapi.h>
#include <errhandlingapi.h>

#include "common.h"
#include "drivers.h"
#include "kbdww.h"
#include "keybd.h"
#include "legacy.h"
#include "memory.h"


FILE* GetOverHandle(FILE* fptr, int diff)
{
	ptrdiff_t pos = find(vOverHandle.begin(), vOverHandle.end(), fptr) - vOverHandle.begin();
	int newPos = pos + diff;
	if (newPos >= 0 && newPos < vOverHandle.size() - 1) { return vOverHandle[pos - 1]; }
	return nullptr;
}

bool IsHandle(filePtr H)
{
	if (H == nullptr) return false;
	return Handles.count(H) > 0;
}

bool IsUpdHandle(filePtr H)
{
	if (H == nullptr) return false;
	return UpdHandles.count(H) > 0;
}

bool IsFlshHandle(filePtr H)
{
	if (H == nullptr) return false;
	return FlshHandles.count(H) > 0;
}

void SetHandle(filePtr H)
{
	if (H == nullptr) return;
	Handles.insert(H);
	CardHandles++;
}

void SetUpdHandle(filePtr H)
{
	if (H == nullptr) return;
	UpdHandles.insert(H);
}

void SetFlshHandle(filePtr H)
{
	if (H == nullptr) return;
	FlshHandles.insert(H);
}

void ResetHandle(filePtr H)
{
	if (H == nullptr) return;
	Handles.erase(H);
	CardHandles--;
}

void ResetUpdHandle(filePtr H)
{
	if (H == nullptr) return;
	UpdHandles.erase(H);
}

void ResetFlshHandle(filePtr H)
{
	if (H == nullptr) return;
	FlshHandles.erase(H);
}

void ClearHandles()
{
	Handles.clear();
	CardHandles = 0;
}

void ClearUpdHandles()
{
	UpdHandles.clear();
}

void ClearFlshHandles()
{
	FlshHandles.clear();
}

bool IsNetCVol()
{
#ifdef FandNetV
	return CVol == "#" || CVol = "##" || SEquUpcase(CVol, "#R");
#else
	return false;
#endif
}

void ExtendHandles()
{
	// pøesouvá OldHTPtr na NewHT
}

void UnExtendHandles()
{
	// zavøe všechny otevøené soubory, pøesune zpìt NewHT do Old... promìnných
}

filePtr OpenH(FileOpenMode Mode, FileUseMode UM)
{
	// $3C vytvoøí nebo pøepíše soubor
	// $3D otevírá exitující soubor
	// $5B vytvoøí nový soubor - pokud již exituje, vyhodí chybu
	//
	// bit 0: read-only, 1: hidden, 2: system, 3: volume label, 4: reserved, must be zero (directory)
	//        5: archive bit, 7: if set, file is shareable under Novell NetWare
	//
	// pøi 'IsNetCVol' se chová jinak
	// RdOnly $20, RdShared $40, Shared $42, Exclusive $12

	pstring s;

	string txt[] = { "Clos", "OpRd", "OpRs", "OpSh", "OpEx" };

	pstring path = CPath;
	if (CardHandles == files) RunError(884);
	longint w = 0;
	pstring openFlags(5);
label1:
	switch (Mode) {
	case _isoldfile:
	case _isoldnewfile:
	{
		openFlags = UM == RdOnly ? "rb" : "r+b";
		break;
	}
	case _isoverwritefile:
	{
		openFlags = "w+b";
		break;
	}
	case _isnewfile:
	{
		openFlags = UM == RdOnly ? "rb" : "r+b";
		break;
	}
	}

	filePtr nFile = nullptr;
	HandleError = fopen_s(&nFile, path.c_str(), openFlags.c_str());

	// https://docs.microsoft.com/en-us/cpp/c-runtime-library/errno-doserrno-sys-errlist-and-sys-nerr?view=vs-2019
	if (IsNetCVol() && (HandleError == EACCES || HandleError == ENOLCK))
	{
		if (w == 0)
		{
			Set2MsgPar(path, txt[UM]);
			w = PushWrLLMsg(825, false);
		}
		Drivers::LockBeep();
		KbdTimer(spec.NetDelay, 0);
		goto label1;
	}

	if (HandleError == 0)
	{
		SetHandle(nFile);
		if (Mode != _isoldfile) SetUpdHandle(nFile);
	}

	else if (HandleError == ENOENT) // No such file or directory
	{
		if (Mode == _isoldnewfile)
		{
			Mode = _isnewfile;
			goto label1;
		}
	}
	if (w != 0) PopW(w);

	// pøidání FILE* od vektoru kvùli 'WORD OvrHandle = h - 1;'
	vOverHandle.push_back(nFile);
	return nFile;
}

WORD ReadH(filePtr handle, WORD bytes, void* buffer)
{
	return ReadH(handle, bytes, buffer);
	// read from file INT
	// bytes - poèet byte k pøeètení
	// vrací - poèet skuteènì pøeètených
}

WORD ReadLongH(filePtr handle, longint bytes, void* buffer)
{
	if (handle == nullptr) RunError(706);
	if (bytes <= 0) return 0;
	auto readed = fread_s(buffer, bytes, 1, bytes, handle);
	if (readed != static_cast<unsigned int>(bytes))
	{
		// nebyl naèten požadovaný poèet B
		auto eofReached = feof(handle);
		HandleError = ferror(handle);
	}
	return WORD(readed);
}

void WriteH(filePtr handle, WORD bytes, void* buffer)
{
	WriteLongH(handle, bytes, buffer);
}

void WriteLongH(filePtr handle, longint bytes, void* buffer)
{
	if (handle == nullptr) RunError(706);
	if (bytes <= 0) return;
	// uloží do souboru daný poèet Bytù z bufferu
	fwrite(buffer, 1, bytes, handle);
	HandleError = ferror(handle);
}

longint MoveH(longint dist, WORD method, filePtr handle)
{
	if (handle == nullptr) return -1;
	// dist - hodnota offsetu
	// method: 0 - od zacatku, 1 - od aktualni, 2 - od konce
	// handle - file handle
	HandleError = fseek(handle, dist, method);
	return ftell(handle);
}

longint PosH(filePtr handle)
{
	const auto result = ftell(handle);
	HandleError = ferror(handle);
	return static_cast<longint>(result);
}

void SeekH(filePtr handle, longint pos)
{
	if (handle == nullptr) RunError(705);
	MoveH(pos, 0, handle);
}

longint FileSizeH(filePtr handle)
{
	longint pos = PosH(handle);
	auto result = MoveH(0, 2, handle);
	SeekH(handle, pos);
	return result;
}

bool TryLockH(filePtr Handle, longint Pos, WORD Len)
{
	return false;
}

bool UnLockH(filePtr Handle, longint Pos, WORD Len)
{
	return false;
}

void TruncH(filePtr handle, longint N)
{
	// cilem je zkratit delku souboru na N
	if (handle == nullptr) return;
	if (FileSizeH(handle) > N) {
		SeekH(handle, N);
		SetEndOfFile(handle);
	}
}

void CloseClearH(filePtr h)
{
	if (h == nullptr) return;
	CloseH(h);
	ClearCacheH(h);
	h = nullptr;
}

void SetFileAttr(WORD Attr)
{
	// nastaví atributy souboru/adresáøe
	// 0 = read only, 1 = hidden file, 2 = system file, 3 = volume label, 4 = subdirectory,
	// 5 = written since backup, 8 = shareable (Novell NetWare)
	if (SetFileAttributesA(CPath.c_str(), Attr) == 0)
	{
		HandleError = GetLastError();
	}
}

WORD GetFileAttr()
{
	// získá atributy souboru/adresáøe
	auto result = GetFileAttributesA(CPath.c_str());
	if (result == INVALID_FILE_ATTRIBUTES) HandleError = GetLastError();
	return result;
}

void RdWrCache(bool ReadOp, filePtr Handle, bool NotCached, longint Pos, WORD N, void* Buf)
{
	if (Handle == nullptr) return;
	// asi netøeba øešit
	return;
}

void CloseH(filePtr handle)
{
	if (handle == nullptr) return;
	// uzavøe soubor
	HandleError = fclose(handle);
}

void FlushH(filePtr& handle)
{
	if (handle == nullptr) return;

	auto result = fflush(handle);
	if (result == EOF) { HandleError = result; }
	//SetHandle(handle);
	SetUpdHandle(handle);
	//CloseH(handle);
}

void FlushHandles()
{
	for (auto handle : UpdHandles)
	{
		FlushH(handle);
	}
	for (auto handle : FlshHandles)
	{
		FlushH(handle);
	}
	ClearUpdHandles();
	ClearFlshHandles();
}

longint GetDateTimeH(filePtr handle)
{
	if (handle == nullptr) return -1;
	// vrátí èas posledního zápisu souboru + datum posledního zápisu souboru
	// 2 + 2 Byte (datum vlevo, èas vpravo)
	FILETIME ft;
	auto result = GetFileTime(handle, nullptr, nullptr, &ft);
	if (result == 0) HandleError = GetLastError();
	return (ft.dwHighDateTime << 16) + ft.dwLowDateTime;
}

void DeleteFile(pstring path)
{
	// smaže soubor - INT $41
	auto result = remove(path.c_str());
	if (result != 0) HandleError = result;
}

void RenameFile56(pstring OldPath, pstring NewPath, bool Msg)
{
	// pøesouvá nebo pøejmenovává soubor
	// potom:
	auto result = rename(OldPath.c_str(), NewPath.c_str());
	if (result != 0) HandleError = result;
	if (Msg && HandleError != 0)
	{
		Set2MsgPar(OldPath, NewPath);
		RunError(829);
	}
}

pstring MyFExpand(pstring Nm, pstring EnvName)
{
	pstring d;
	GetDir(0, d);
	pstring f = FandDir;
	DelBackSlash(f);
	ChDir(f);
	pstring p = GetEnv(EnvName.c_str());
	AddBackSlash(p);
	if (!p.empty()) p += Nm;
	else {
		pstring envp = GetEnv("PATH");
		p = FSearch(Nm, envp);
		if (p.empty()) p = Nm;
	}
	ChDir(d);
	return p;
}
