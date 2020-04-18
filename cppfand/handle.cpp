#pragma once

#include "handle.h"

#include "common.h"
#include "legacy.h"
#include "memory.h"


bool IsHandle(WORD H)
{
	return H != 0xFF && (Handles.count(H) > 0);
}

bool IsUpdHandle(WORD H)
{
	return H != 0xFF && UpdHandles.count(H) > 0;
}

bool IsFlshHandle(WORD H)
{
	return H != 0xFF && FlshHandles.count(H) > 0;
}

void SetHandle(WORD H)
{
	if (H == 0xFF) return;
	Handles.insert(H);
	CardHandles++;
}

void SetUpdHandle(WORD H)
{
	if (H == 0xFF) return;
	UpdHandles.insert(H);
}

void SetFlshHandle(WORD H)
{
	if (H == 0xFF) return;
	FlshHandles.insert(H);
}

void ResetHandle(WORD H)
{
	if (H == 0xFF) return;
	Handles.erase(H);
	CardHandles--;
}

void ResetUpdHandle(WORD H)
{
	if (H == 0xFF) return;
	UpdHandles.erase(H);
}

void ResetFlshHandle(WORD H)
{
	if (H == 0xFF) return;
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

WORD OpenH(FileOpenMode Mode, FileUseMode UM)
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
}

WORD ReadH(WORD handle, WORD bytes, void* buffer)
{
	if (handle == 0xff) RunError(706);

	// read from file INT
	// bytes - poèet byte k pøeètení
	// vrací - poèet skuteènì pøeètených
	
	return 0;
}

WORD ReadLongH(WORD handle, longint bytes, void* buffer)
{
	return ReadH(handle, bytes, buffer);
}


longint MoveH(longint dist, WORD method, WORD handle)
{
	// dist - hodnota offsetu
	// method: 0 - od zacatku, 1 - od aktualni, 2 - od konce
	// handle - file handle
	return -1;
}

longint PosH(WORD handle)
{
	return MoveH(0, 1, handle);
}

void SeekH(WORD handle, longint pos)
{
	if (handle == 0xff) RunError(705);
	MoveH(pos, 0, handle);
}

longint FileSizeH(WORD handle)
{
	longint pos = PosH(handle);
	auto result = MoveH(0, 2, handle);
	SeekH(handle, pos);
	return result;
}

void TruncH(WORD handle, longint N)
{
	// posune se na pozici N a nic na ni nezapíše? WTF?
	if (handle == 0xff) return;
	if (FileSizeH(handle) > N) {
		SeekH(handle, N);
		WriteH(handle, 0, (void*)&"");
	}
}

void CloseClearH(WORD& h)
{
	if (h == 0xFF) return;
	CloseH(h);
	ClearCacheH(h);
	h = 0xFF;
}

void SetFileAttr(WORD Attr)
{
	// nastaví atributy souboru/adresáøe
	// 0 = read only, 1 = hidden file, 2 = system file, 3 = volume label, 4 = subdirectory,
	// 5 = written since backup, 8 = shareable (Novell NetWare)
}

WORD GetFileAttr()
{
	// získá atributy souboru/adresáøe
	return 0;
}

void RdWrCache(bool ReadOp, WORD Handle, bool NotCached, longint Pos, WORD N, void* Buf)
{
	// asi netøeba øešit
	return;
}

void WriteH(WORD handle, WORD bytes, void* buffer)
{
	if (handle == 0xff) RunError(706);
	// uloží do souboru daný poèet Bytù z bufferu
}

void WriteLongH(WORD handle, longint bytes, void* buffer)
{
	WriteH(handle, bytes, buffer);
}

void CloseH(WORD handle)
{
	// uzavøe soubor
}

void FlushH(WORD handle)
{
	// k èemu to všechno?
	// zažádá o nový handle N
	// zavolá SetHandle(N); SetUpdHandle(H); CloseH(H);
}

void FlushHandles()
{
	if (CardHandles == files) return;
	for (int i=0; i<files; i++)
	{
		if (IsUpdHandle(i) || IsFlshHandle(i)) FlushH(i);
	}
	ClearUpdHandles();
	ClearFlshHandles();
}

longint GetDateTimeH(WORD handle)
{
	// vrátí èas posledního zápisu souboru + datum posledního zápisu souboru
	// 2 + 2 Byte (datum vlevo, èas vpravo)
	return 0;
}

void DeleteFile(pstring path)
{
	// smaže soubor - INT $41
}

void RenameFile56(pstring OldPath, pstring NewPath, bool Msg)
{
	// pøesouvá nebo pøejmenovává soubor
	// potom:
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
		p = FSearch(Nm, GetEnv("PATH"));
		if (p.empty()) p = Nm; 
	}
	ChDir(d);
	return p;
}
