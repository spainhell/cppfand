#include "TzFile.h"
#include "../cppfand/GlobalVariables.h"
#include "../cppfand/oaccess.h"
#include "../cppfand/obaseww.h"

TzFile::TzFile(bool BkUp, bool NoCompr, bool SubDirO, bool OverwrO, WORD Ir, pstring aDir)
{
	SaveFiles();
	ForAllFDs(ClosePassiveFD);
	//if NoCompr then inherited Init(0) else inherited Init(1);
	IsBackup = BkUp;
	SubDirOpt = SubDirO;
	OverwrOpt = OverwrO;
	OldDir = GetDir(0);
	Vol = RdCatField(Ir, CatVolume);
	CPath = RdCatField(Ir, CatPathName); Path = FExpand(CPath); DrvNm = Path[0];
	Dir = aDir; AddBackSlash(Dir);
	WBase = MaxWSize;
	WPos = WBase;
}

void TzFile::Close()
{
	if (Handle != nullptr) {
		MaxWSize = WBase;
		TruncH(WorkHandle, MaxWSize);
		FlushH(WorkHandle);
		CloseH(&Handle);
		ChDir(OldDir);
	}
}

longint TzFile::GetWPtr()
{
	longint result = WPos - WBase;
	WPos += 4;
	return result;
}

void TzFile::StoreWPtr(longint Pos, longint N)
{
	SeekH(WorkHandle, WBase + Pos);
	WriteH(WorkHandle, 4, &N);
}

longint TzFile::StoreWStr(pstring s)
{
	SeekH(WorkHandle, WPos);
	WriteH(WorkHandle, s.length() + 1, &s);
	longint result = WPos - WBase;
	WPos += s.length() + 1;
	return result;
}

longint TzFile::ReadWPtr(longint Pos)
{
	longint n;
	SeekH(WorkHandle, WBase + Pos);
	ReadH(WorkHandle, 4, &n);
	return n;
}

pstring TzFile::ReadWStr(longint& Pos)
{
	pstring s;
	SeekH(WorkHandle, WBase + Pos);
	ReadH(WorkHandle, 1, &s);
	ReadH(WorkHandle, s.length(), &s[1]);
	Pos += s.length() + 1;
	return s;
}

longint TzFile::StoreDirD(std::string RDir)
{
	longint result = GetWPtr();
	GetWPtr();
	GetWPtr();
	StoreWStr(RDir);
	return result;
}

void TzFile::SetDir(std::string RDir)
{
	std::string d = Dir + RDir;
	SetMsgPar(d);
	DelBackSlash(d);
label1:
	ChDir(d);
	if (IOResult() != 0)
		if (!IsBackup && SubDirOpt) {
			MkDir(d);
			if (IOResult() != 0) RunError(644);
			goto label1;
		}
		else RunError(703);
}

void TzFile::Get1Dir(StringList Msk, longint D, longint& DLast)
{
	/*SearchRec SR;
	PathStr p;
	longint i, n;
	pstring RDir;
	StringList sl;

	i = D + 12;
	RDir = ReadWStr(i);
	p = Dir + RDir + "*.*";
	FindFirst(p + 00, 0, SR);
	n = 0;
	if (!(DosError() == 0 || DosError() == 18)) {
		SetMsgPar(p);
		RunError(904);
	}
	while (DosError() == 0) {
		if (Msk == nullptr) {
		label1:
			i = StoreWStr(SR.name);
			if (n == 0) StoreWPtr(D + 4, i);
			n++;
		}
		else {
			sl = Msk;
			while (sl != nullptr) {
				if (EqualsMask(SR.name[1], length(SR.name), sl->S)) goto label1;
				sl = (StringListEl*)sl->pChain;
			}
		}
		FindNext(SR);
	}
	StoreWPtr(D + 8, n);
	StoreWPtr(DLast, 0);
	if (!SubDirOpt) return;
	FindFirst(p + 00, Directory, SR);
	while (DosError() == 0) {
		if (((SR.Attr && Directory) == Directory) && (SR.name[1] != '.')) {
			i = StoreDirD(RDir + SR.name + "\'");
			StoreWPtr(DLast, i);
			DLast = i;
		}
		FindNext(SR);
	}*/
}

void TzFile::GetDirs(LongStr* Mask)
{
	StringList slRoot, sl; pstring s; longint d, dLast; WORD l, i, j, n;

	slRoot = nullptr;
	l = Mask->LL;
	j = 1;
	do {
		while ((j <= l) && (Mask->A[j] == ' ' || Mask->A[j] == ',')) { j++; }
		n = l - j + 1;
		for (i = j; i <= l; i++) {
			if (Mask->A[i] == ' ' || Mask->A[i] == ',') { n = i - j; break; }
			if (n > 0) {
				n = MinW(n, 255);
				sl = new StringListEl(); // GetZStore(5 + n);
				ChainLast(slRoot, sl);
				Move(&Mask->A[j], &sl->S[1], n);
				sl->S[0] = char(n);
				j += n;
			}
		}
	} while (n != 0);
	d = StoreDirD("");
	dLast = d;
	do {
		Get1Dir(slRoot, d, dLast);
		d = ReadWPtr(d);
	} while (d != 0);
	SeekH(WorkHandle, WBase);
	WrH(WorkHandle, WPos - WBase);
}

void TzFile::Reset()
{
	CVol = Vol;
	CPath = Path;
	Handle = OpenH(_isoldfile, RdOnly);
	Continued = false;
	/*if ((HandleError == 2) && Floppy) {
		inc(CPath[l - 2], 5);
		Handle = OpenH(_isoldfile, RdOnly);
		Continued = true;
	}*/
	TestCPathError();
	Size = FileSizeH(Handle);
	OrigSize = Size;
	RunMsgOn('C', Size);
}

void TzFile::Rewrite()
{
	CVol = Vol;
	CPath = Path;
	Handle = OpenH(_isoverwritefile, Exclusive);
	TestCPathError();
	SpaceOnDisk = MyDiskFree(Floppy, Drive);
}

void TzFile::RdH(FILE* H, bool Skip)
{
	longint sz;
	BYTE* a = (BYTE*)&sz;

	for (WORD i = 0; i <= 3; i++) {
		if (iBuf == lBuf) ReadBuf();
		a[i] = Buf[iBuf];
		iBuf++;
	}
	while (sz > 0) {
		if (iBuf == lBuf) ReadBuf();
		WORD n = lBuf - iBuf;
		if (sz < n) n = sz;
		if (!Skip) WriteH(H, n, &Buf[iBuf]);
		iBuf += n;
		sz -= n;
	}
}

void TzFile::WrH(FILE* H, longint Sz)
{
	Move(&Sz, Buf, 4);
	WORD j = 4;
	WORD max = BufSize - 4;
	RunMsgOn('C', Sz);
	longint i = 0;
	do {
		WORD n;
		if (Sz - i > max) n = max;
		else n = Sz - i;
		i += n;
		if (n > 0) ReadH(H, n, &Buf[j]);
		lBuf = j + n;
		WriteBuf(false);
		j = 0;
		max = BufSize;
		RunMsgN(i);
	} while (i != Sz);

	RunMsgOff();
}

void TzFile::ProcFileList()
{
	void* p = nullptr;
	FILE* h = nullptr;

	longint d = 0;
	MarkStore(p);
	do {
		longint dNext = ReadWPtr(d);
		longint n = ReadWPtr(d + 8);
		longint i = d + 12;
		pstring RDir = ReadWStr(i);
		SetDir(RDir);
		i = ReadWPtr(d + 4);
		while (n > 0) {
			pstring FName = ReadWStr(i);
			n--;
			CPath = FExpand(FName);
			CVol = "";
			if (IsBackup) {
				h = OpenH(_isoldfile, RdOnly);
				TestCPathError();
				WrH(h, FileSizeH(h));
			}
			else {
				bool skp = false;
				if (!OverwrOpt) {
					h = OpenH(_isnewfile, Exclusive);
					if (HandleError == 80) {
						SetMsgPar(CPath);
						if (PromptYN(780)) h = OpenH(_isoverwritefile, Exclusive);
						else {
							skp = true;
							goto label1;
						}
					}
				}
				else h = OpenH(_isoverwritefile, Exclusive);
				TestCPathError();
			label1:
				RdH(h, skp);
			}
			CloseH(&h);
		}
		d = dNext;
		ReleaseStore(p);
	} while (d != 0);
}

void TzFile::Backup(LongStr* aMsk)
{
	MountVol(true);
	Rewrite();
	InitBufOutp();
	GetDirs(aMsk);
	ProcFileList();
	WriteBuf(true);
}

void TzFile::Restore()
{
	MountVol(true);
	Reset();
	if (Size == 0) CloseH(&Handle);
	else {
		InitBufInp();
		SeekH(WorkHandle, WBase);
		RdH(WorkHandle, false);
		ProcFileList();
	}
	RunMsgOff();
}
