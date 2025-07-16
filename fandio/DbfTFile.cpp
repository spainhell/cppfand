#include "DbfTFile.h"

#include <cstdlib>
#include <cstring>
#include "../Core/access.h"
#include "../Core/obaseww.h"
#include "../Core/GlobalVariables.h"

DbfTFile::DbfTFile(DbfFile* parent) : DataFileBase(parent->get_callbacks())
{
	_parent = parent;
}

DbfTFile::~DbfTFile()
{
}

void DbfTFile::Err(unsigned short n, bool ex) const
{
	FileMsg(_parent->GetFileD(), n, 'T');
	if (ex) {
		_parent->GetFileD()->Close();
		GoExit(MsgLine);
	}
}

void DbfTFile::TestErr() const
{
	if (HandleError != 0) Err(700 + HandleError, true);
}

int DbfTFile::UsedFileSize() const
{
	if (Format == FptFormat) {
		return FreePart * FptFormatBlockSize;
	}
	else {
		return int(MaxPage + 1) << 9; // 9 = MPageShft;
	}
}

void DbfTFile::RdPrefix(bool check)
{
	//TODO: FandTFilePrefix T;
	uint8_t T[512];

	int* TNxtAvailPage = (int*)&T; /* .DBT */
	struct stFptHd { int FreePart = 0; unsigned short X = 0, BlockSize = 0; }; /* .FPT */
	stFptHd* FptHd = (stFptHd*)&T;
	unsigned char sum = 0;
	int FS = 0, ML = 0, RS = 0;
	unsigned short i = 0, n = 0;
	unsigned char header512[512];
	ReadData(0, 512, header512);
	srand(RS);

	if (Format == DbtFormat) {
		MaxPage = *TNxtAvailPage - 1;
		GetMLen();
		return;
	}
	if (Format == FptFormat) {
		//FreePart = SwapLong((*FptHd).FreePart);
		FptFormatBlockSize = Swap((*FptHd).BlockSize);
		return;
	}
}

void DbfTFile::WrPrefix()
{
	//TODO: FandTFilePrefix T;
	uint8_t T[512];

	switch (Format) {
	case DbtFormat: {
		int* TNxtAvailPage = (int*)&T;		/* .DBT */
		memset(&T, ' ', sizeof(T));
		*TNxtAvailPage = MaxPage + 1;
		break;
	}
	case FptFormat: {
		struct stFptHd { int FreePart = 0; unsigned short X = 0, BlockSize = 0; }; /* .FPT */
		stFptHd* FptHd = (stFptHd*)&T;
		memset(&T, 0, sizeof(T));
		//(*FptHd).FreePart = SwapLong(FreePart);
		//(*FptHd).BlockSize = Swap(FptFormatBlockSize);
		break;
	}
	}
}

void DbfTFile::SetEmpty()
{
	switch (Format) {
	case DbtFormat: {
		MaxPage = 0;
		WrPrefix();
		break;
	}
	case FptFormat: {
		FreePart = 8;
		FptFormatBlockSize = 64;
		WrPrefix();
		break;
	}
	}
}

std::string DbfTFile::Read(int32_t pos)
{
	std::string s;
	pos -= LicenseNr;
	if (pos <= 0) {
		// OldTxt=-1 in RDB!
		// return empty string
	}
	else {
		switch (Format) {
		case DbtFormat: {
			LongStr* s = new LongStr(32768); //(LongStr*)GetStore(32770);
			pos = pos << MPageShft;
			unsigned l = 0;
			char* p = s->A;
			int offset = 0;

			while (l <= 32768 - MPageSize) {
				ReadData(pos, MPageSize, &p[offset]);
				for (uint16_t i = 1; i <= MPageSize; i++) {
					if (p[offset + i] == 0x1A) {
						s->LL = l;
						//ReleaseStore(&s->A[l + 1]);
						//return s;
						return "";
					}
					l++;
				}
				offset += MPageSize;
				pos += MPageSize;
			}
			l--;
			s->LL = l;
			//ReleaseStore(&s->A[l + 1]);
			break;
			break;
		}
		case FptFormat: {
			struct { int Typ = 0, Len = 0; } FptD;
			pos = pos * FptFormatBlockSize;
			ReadData(pos, sizeof(FptD), &FptD);
			s = "";
			break;
		}
		}
	}

	return s;
}

uint32_t DbfTFile::Store(const std::string& data)
{
	uint8_t* s = (uint8_t*)data.c_str();
	size_t l = data.length();

	uint32_t pos = 0;
	char X[MPageSize + 1]{ 0 };
	struct stFptD { int Typ = 0, Len = 0; } FptD;

	if (l == 0) {
		return pos;
	}

	SetUpdateFlag();

	switch (Format) {
	case DbtFormat: {
		pos = MaxPage + 1;
		int N = pos << MPageShft;
		if (l > 0x7fff) l = 0x7fff;
		WriteData(N, l, s);
		FillChar(X, MPageSize, ' ');
		X[0] = 0x1A; X[1] = 0x1A;
		int rest = MPageSize - (l + 2) % MPageSize;
		WriteData(N + l, rest + 2, X);
		MaxPage += (l + 2 + rest) / MPageSize;
		break;
	}
	case FptFormat: {
		pos = FreePart;
		int N = FreePart * FptFormatBlockSize;
		if (l > 0x7fff) l = 0x7fff;
		FreePart = FreePart + (sizeof(FptD) + l - 1) / FptFormatBlockSize + 1;
		//FptD.Len = SwapLong(l);
		WriteData(N, sizeof(FptD), &FptD);
		N += sizeof(FptD);
		WriteData(N, l, s);
		N += l;
		l = FreePart * FptFormatBlockSize - N;
		if (l > 0) {
			unsigned char* p = new unsigned char[l];
			FillChar(p, l, ' ');
			WriteData(N, l, p);
			delete[] p; p = nullptr;
		}
		break;
	}
	}
	return pos;
}

void DbfTFile::Delete(int32_t pos)
{
	// for DBF this is not implemented
}

void DbfTFile::Create(const std::string& path)
{
	Handle = OpenH(path, _isOverwriteFile, Exclusive);
	TestErr();
	//IRec = 1;
	LicenseNr = 0;

	/*PwCode = "";
	PwCode = AddTrailChars(PwCode, '@', 20);
	PwCode = Coding::Code(PwCode);

	Pw2Code = "";
	Pw2Code = AddTrailChars(Pw2Code, '@', 20);
	Pw2Code = Coding::Code(Pw2Code);*/

	eofPos = 2 * MPageSize;

	SetEmpty();
}

void DbfTFile::CloseFile()
{
	if (Handle != nullptr) {
		CloseClearH(&Handle);
		if (HandleError == 0) {
			Handle = nullptr;
			ClearUpdateFlag();
		}
	}
}

void DbfTFile::ClearUpdateFlag()
{
	DataFileBase::ClearUpdateFlag();
}

void DbfTFile::GetMLen()
{
	MLen = (MaxPage + 1) << 9; // 9 = MPageShft
}

