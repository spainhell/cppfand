#include "TzFile.h"
#include "../Core/GlobalVariables.h"
#include "../Core/oaccess.h"
#include "../Core/obaseww.h"
#include "../Core/RunMessage.h"

TzFile::TzFile(bool BkUp, bool NoCompr, bool SubDirO, bool OverwrO, int Ir, pstring aDir)
{
	SaveAndCloseAllFiles();
	ForAllFDs(ForAllFilesOperation::close_passive_fd);
	//if NoCompr then inherited init(0) else inherited init(1);
	IsBackup = BkUp;
	SubDirOpt = SubDirO;
	OverwrOpt = OverwrO;
	OldDir = GetDir(0);
	Vol = CatFD->GetVolume(Ir);
	CPath = CatFD->GetPathName(Ir);
	Path = FExpand(CPath);
	DrvNm = Path[0];
	Dir = aDir;
	AddBackSlash(Dir);
	WBase = MaxWSize;
	WPos = WBase;
}

void TzFile::Close()
{
	if (Handle != nullptr) {
		MaxWSize = WBase;
		TruncF(WorkHandle, HandleError, MaxWSize);
		FlushF(WorkHandle, HandleError);
		CloseH(&Handle);
		ChDir(OldDir);
	}
}

int TzFile::GetWPtr()
{
	int result = WPos - WBase;
	WPos += 4;
	return result;
}

void TzFile::StoreWPtr(int Pos, int N)
{
	SeekH(WorkHandle, WBase + Pos);
	WriteH(WorkHandle, 4, &N);
}

int TzFile::StoreWStr(pstring s)
{
	SeekH(WorkHandle, WPos);
	WriteH(WorkHandle, s.length() + 1, &s);
	int result = WPos - WBase;
	WPos += s.length() + 1;
	return result;
}

int TzFile::ReadWPtr(int Pos)
{
	int n;
	SeekH(WorkHandle, WBase + Pos);
	ReadH(WorkHandle, 4, &n);
	return n;
}

pstring TzFile::ReadWStr(int& Pos)
{
	pstring s;
	SeekH(WorkHandle, WBase + Pos);
	ReadH(WorkHandle, 1, &s);
	ReadH(WorkHandle, s.length(), &s[1]);
	Pos += s.length() + 1;
	return s;
}

int TzFile::StoreDirD(std::string RDir)
{
	int result = GetWPtr();
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
	if (IOResult() != 0) {
		if (!IsBackup && SubDirOpt) {
			MkDir(d);
			if (IOResult() != 0) RunError(644);
			goto label1;
		}
	}
	else {
		RunError(703);
	}
}

void TzFile::Get1Dir(StringList Msk, int D, int& DLast)
{
	/*SearchRec SR;
	PathStr p;
	int i, n;
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
	StringList slRoot, sl;
	pstring s;
	int d, dLast;
	WORD l, i, j, n;

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
	Handle = OpenH(CPath, _isOldFile, RdOnly);
	Continued = false;
	/*if ((HandleError == 2) && Floppy) {
		inc(CPath[l - 2], 5);
		Handle = OpenH(_isOldFile, RdOnly);
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
	Handle = OpenH(CPath, _isOverwriteFile, Exclusive);
	TestCPathError();
	SpaceOnDisk = MyDiskFree(Floppy, Drive);
}

void TzFile::RdH(HANDLE H, bool Skip)
{
	int sz;
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

void TzFile::WrH(HANDLE H, int Sz)
{
	Move(&Sz, Buf, 4);
	WORD j = 4;
	WORD max = BufSize - 4;
	RunMsgOn('C', Sz);
	int i = 0;
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
	HANDLE h = nullptr;

	int d = 0;
	MarkStore(p);
	do {
		int dNext = ReadWPtr(d);
		int n = ReadWPtr(d + 8);
		int i = d + 12;
		pstring RDir = ReadWStr(i);
		SetDir(RDir);
		i = ReadWPtr(d + 4);
		while (n > 0) {
			pstring FName = ReadWStr(i);
			n--;
			CPath = FExpand(FName);
			CVol = "";
			if (IsBackup) {
				h = OpenH(CPath, _isOldFile, RdOnly);
				TestCPathError();
				WrH(h, FileSizeH(h));
			}
			else {
				bool skp = false;
				if (!OverwrOpt) {
					h = OpenH(CPath, _isNewFile, Exclusive);
					if (HandleError == 80) {
						SetMsgPar(CPath);
						if (PromptYN(780)) h = OpenH(CPath, _isOverwriteFile, Exclusive);
						else {
							skp = true;
							goto label1;
						}
					}
				}
				else h = OpenH(CPath, _isOverwriteFile, Exclusive);
				TestCPathError();
			label1:
				RdH(h, skp);
			}
			CloseH(&h);
		}
		d = dNext;
		ReleaseStore(&p);
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
