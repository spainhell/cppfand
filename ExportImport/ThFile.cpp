#include "ThFile.h"
#include "../Drivers/constants.h"
#include "../Core/GlobalVariables.h"
#include "../Core/oaccess.h"
#include "../Core/obaseww.h"
#include "../Core/RunMessage.h"

WORD iBuf = 0;

ThFile::ThFile(std::string APath, WORD CatIRec, InOutMode AMode, byte aCompress, ThFile* F) : TcFile(aCompress)
{
	mode = AMode;
	DWORD access_mode = 0;
	DWORD create_mode = 0;
	std::string d, Nm, e;

	switch (mode) {
	case InOutMode::_inp: {
		access_mode = GENERIC_READ;
		create_mode = OPEN_EXISTING;
		break;
	}
	case InOutMode::_outp: {
		access_mode = GENERIC_READ | GENERIC_WRITE;
		create_mode = CREATE_ALWAYS;
		break;
	}
	case InOutMode::_append: {
		access_mode = GENERIC_READ | GENERIC_WRITE;
		create_mode = OPEN_ALWAYS;
		break;
	}
	default: break;
	}

	Compress = aCompress;

	SetTxtPathVol(APath, CatIRec);
	Path = CPath;
	Vol = CVol;

	if (F != nullptr && F->Path == CPath) {
		SetMsgPar(CPath);
		RunError(660);
	}

	//if (CVol != "#" && CVol != "" && pos('.', CPath) == l - 3 && CPath[l] >= '0' && CPath[l] <= '9' && CPath[l - 1] >= '0' && CPath[l - 1] <= '4') Floppy = true;
	FSplit(CPath, d, Nm, e);
	//ForAllFDs(CloseEqualFD);

	Handle = OpenF(CPath, HandleError, access_mode, 0, create_mode);

	if (Handle != INVALID_HANDLE_VALUE) {
		if (HandleError == ERROR_ALREADY_EXISTS) {
			HandleError = 0;
		}
		this->ReadBuf();
	}
	else {
		if (HandleError == 2) {
			Size = 0;
			eof = true;
		}
	}

	if (AMode == InOutMode::_inp) {
		RunMsgOn('C', Size);
	}
	if (AMode == InOutMode::_append) {
		SeekF(Handle, HandleError, 0, FILE_END);
	}
}

ThFile::~ThFile()
{
	if (Handle != INVALID_HANDLE_VALUE) {
		if (mode == InOutMode::_inp) {
			RunMsgOff();
		}
		else {
			WriteBuf(true);
			if (!Continued && Size == 0) {
				DeleteFile(Path.c_str());
			}
		}
		CloseH(&Handle);

		Handle = INVALID_HANDLE_VALUE;
	}
}

void ThFile::ReadBuf()
{
	if (Handle != INVALID_HANDLE_VALUE) {
		lBuf = ReadH(Handle, BUFFER_SIZE, Buf);
		if (lBuf == 0) this->eof = true;
		Size = lBuf;
	}
	else {
		Size = 0;
		eof = true;
	}
}

void ThFile::WriteBuf(bool cond)
{
	if (Handle != INVALID_HANDLE_VALUE) {
		//lBuf = fwrite(Buf, 1, lBuf, Handle);
		WriteH(Handle, lBuf, Buf);
	}
}

void ThFile::ClearBuf()
{
	memset(Buf, 0, sizeof(*Buf));
	lBuf = 16384;
}

void ThFile::Delete()
{
	CVol = Vol;
	CPath = Path;
	TestMountVol(CPath[0]);
	do { DeleteFile(CPath.c_str()); } while (TestErr152());
	//if (Floppy) { CPath[l - 1] += 5); DeleteFile(CPath); }
}

char ThFile::ForwChar()
{
	char result;
label1:
	if (iBuf < lBuf) result = (char)Buf[iBuf];
	else {
		ReadBuf();
		if (eof) result = 0x1A;
		else goto label1;
	}
	return result;
}

char ThFile::RdChar()
{
	char result;
label1:
	if (iBuf < lBuf) {
		result = (char)Buf[iBuf];
		iBuf++;
	}
	else {
		ReadBuf();
		if (eof) result = 0x1A;
		else goto label1;
	}
	return result;
}

std::string ThFile::RdDM(char Delim, short Max)
{
	std::string s;
	IsEOL = false;
	char c = RdChar();
label1:
	if (eof) IsEOL = true;
	else {
		if ((c == '\r') && !(Delim == '\'' || Delim == '"')) {
			IsEOL = true;
			if (ForwChar() == '\n') RdChar();
		}
		else {
			if (c != Delim) {
			label2:
				s += c;
				if (s.length() < Max) {
					c = RdChar();
					goto label1;
				}
			}
			else if ((Delim == '\'' || Delim == '"') && (ForwChar() == c)) {
				RdChar();
				goto label2;
			}
		}
	}
	return s;
}

std::string ThFile::RdDelim(char Delim)
{
	return RdDM(Delim, 255);
}

std::string ThFile::RdFix(short N)
{
	return RdDM('\r', N);
}

std::string ThFile::RdLongStr()
{
	char c; WORD l, n;

	std::string x;
	l = 0; n = 0;
	if (ForwChar() == '\'') {
		RdChar();
		c = RdChar();
		while (!eof && (l < MaxLStrLen)) {
			if (c == '\'') {
				if (ForwChar() == '\'') RdChar();
				else goto label1;
			}
			x += c;
			c = RdChar();
		}
	}
	else
		while (!eof && (l < MaxLStrLen) && (ForwChar() != '\r')) {
			x += RdChar();
		}
label1:
	return x;
}

void ThFile::ExtToT()
{
	CPath = Path;
	FSplit(CPath, CDir, CName, CExt);
	CPath = CExtToT(nullptr, CDir, CName, CExt);
	Path = CPath;
}

void ThFile::Rewrite()
{
	Delete();
	CPath = Path;
	CVol = Vol;
	Handle = OpenH(CPath, _isOverwriteFile, Exclusive);
	TestCPathError();
	SpaceOnDisk = MyDiskFree(Floppy, Path[0] - '@');
	Size = 0;
}

void ThFile::RewriteT()
{
	ExtToT();
	Rewrite();
	InitBufOutp();
}

void ThFile::RewriteX()
{
	Path[Path.length() - 3] = 'X';
	Rewrite();
	InitBufOutp();
}

bool ThFile::TestErr152()
{
	bool result = false;
	if (HandleError == 152) {
		F10SpecKey = __ESC;

		SetMsgPar(CPath);
		MsgPar[1] = "";

		WrLLF10Msg(808);
		const WORD KbdChar = Event.Pressed.KeyCombination();
		if ((KbdChar == __ESC) && PromptYN(21)) GoExit(MsgLine);
		result = true;
	}
	return result;
}

void ThFile::WrChar(char C)
{
	if (lBuf == BUFFER_SIZE) WriteBuf(false);
	Buf[lBuf] = C;
	lBuf++;
}

void ThFile::WrString(std::string S)
{
	for (size_t i = 0; i < S.length(); i++) WrChar(S[i]);
}

void ThFile::WrLongStr(LongStr* S, bool WithDelim)
{
	if (WithDelim) WrChar('\'');
	for (size_t i = 0; i < S->LL; i++) {
		WrChar(S->A[i]);
		if (WithDelim && (S->A[i] == '\'')) WrChar('\'');
	}
	if (WithDelim) WrChar('\'');
}

