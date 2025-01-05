#include "FandTFile.h"

#include <memory>

#include "FandTFilePrefix.h"
#include "files.h"
#include "../Core/Coding.h"
#include "../Core/FileD.h"
#include "../Core/GlobalVariables.h"
#include "../Core/obaseww.h"
#include "../pascal/random.h"
#include "../Common/textfunc.h"
#include "../Common/compare.h"


FandTFile::FandTFile(Fand0File* parent)
{
	_parent = parent;
}

FandTFile::FandTFile(const FandTFile& orig, Fand0File* parent)
{
	_parent = parent;
	Format = orig.Format;
}

FandTFile::~FandTFile()
{
	if (Handle != nullptr) {
		CloseH(&Handle);
	}
}

void FandTFile::Err(unsigned short n, bool ex) const
{
	if (IsWork) {
		SetMsgPar(FandWorkTName);
		WrLLF10Msg(n);
		if (ex) GoExit(MsgLine);
	}
	else {
		FileMsg(_parent->GetFileD(), n, 'T');
		if (ex) CloseGoExit(_parent->GetFileD()->FF);
	}
}

void FandTFile::TestErr() const
{
	if (HandleError != 0) Err(700 + HandleError, true);
}

int FandTFile::UsedFileSize() const
{
	if (Format == FptFormat) {
		return FreePart * FptFormatBlockSize;
	}
	else {
		return int(MaxPage + 1) << MPageShft;
	}
}

bool FandTFile::NotCached() const
{
	return !IsWork && _parent->NotCached();
}

bool FandTFile::Cached() const
{
	return !NotCached();
}

void FandTFile::RdPrefix(bool check)
{
	FandTFilePrefix T;
	int* TNxtAvailPage = (int*)&T; /* .DBT */
	struct stFptHd { int FreePart = 0; unsigned short X = 0, BlockSize = 0; }; /* .FPT */
	stFptHd* FptHd = (stFptHd*)&T;
	unsigned char sum = 0;
	int FS = 0, ML = 0, RS = 0;
	unsigned short i = 0, n = 0;
	if (check) {
		FS = FileSizeH(Handle);
		if (FS <= 512) {
			//FillChar(PwCode, 40, '@');
			PwCode = "";
			PwCode = AddTrailChars(PwCode, '@', 40);
			PwCode = Coding::Code(PwCode);
			SetEmpty();
			return;
		}
	}
	unsigned char header512[512];
	ReadData(0, 512, header512);
	srand(RS);
	LicenseNr = 0;
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

	// nactena data jsou porad v poli, je nutne je nahrat do T
	T.Load(header512);

	FreePart = T.free_part; // 4B
	Reserved = T.rsrvd1; // 1B
	CompileProc = T.CompileProc; // 1B
	CompileAll = T.CompileAll; // 1B
	IRec = T.IRec; // 2B
	FreeRoot = T.FreeRoot; // 4B
	MaxPage = T.MaxPage; // 4B
	TimeStmp = T.TimeStmp; // 6B v Pascalu, 8B v C++ 

	if (!IsWork
		&& (_parent->GetFileD() == Chpt)
		&& (/*(T.HasCoproc != HasCoproc) ||*/ (CompArea(Version, T.Version, 4) != _equ))) {
		CompileAll = true;
	}
	if (T.old_max_page == 0xffff) {
		GetMLen();
		ML = MLen;
		if (!check) FS = ML;
	}
	else {
		FreeRoot = 0;
		if (FreePart > 0) {
			if (!check) FS = FileSizeH(Handle);
			ML = FS;
			MaxPage = (FS - 1) >> MPageShft;
			GetMLen();
		}
		else {
			FreePart = -FreePart;
			MaxPage = T.old_max_page;
			GetMLen();
			ML = MLen;
			if (!check) FS = ML;
		}
	}
	if (IRec >= 0x6000) {
		IRec = IRec - 0x2000;
		if (!IsWork && (_parent->file_type == FileType::RDB)) LicenseNr = T.LicNr;
	}
	if (IRec >= 0x4000) {
		IRec = IRec - 0x4000;
		RandSeed = ML + T.Time;
		RandIntByBytes(T.FreeRoot);
		RandIntByBytes(T.MaxPage);
		RandReal48ByBytes(T.TimeStmp);
		RandBooleanByBytes(T.HasCoproc);
		RandArrayByBytes(T.Rsrvd2, 25);
		RandArrayByBytes(T.Version, 4);
		RandArrayByBytes(T.LicText, 105);
		RandArrayByBytes(&T.Sum, 1);
		RandArrayByBytes(T.X1, 295);
		RandByteByBytes(T.LicNr);
		RandArrayByBytes(T.X2, 11);
		RandArrayByBytes(T.PwNew, 40);
		PwCode = std::string(&T.PwNew[0], 20);
		Pw2Code = std::string(&T.PwNew[20], 20);
	}
	else {
		RandSeed = ML + T.Time;
		RandIntByBytes(T.FreeRoot);
		RandIntByBytes(T.MaxPage);
		RandReal48ByBytes(T.TimeStmp);
		RandBooleanByBytes(T.HasCoproc);
		RandArrayByBytes(T.Rsrvd2, 25);
		PwCode = std::string(&T.PwNew[0], 20);
		Pw2Code = std::string(&T.PwNew[20], 20);
	}
	PwCode = Coding::Code(PwCode);
	Pw2Code = Coding::Code(Pw2Code);
	if ((FreePart < MPageSize) || (FreePart > ML) || (FS < ML) ||
		(FreeRoot > MaxPage) || (MaxPage == 0)) {
		Err(893, false);
		MaxPage = (FS - 1) >> MPageShft;
		FreeRoot = 0;
		GetMLen();
		FreePart = NewPage(true);
		SetUpdateFlag(); //SetUpdHandle(Handle);
	}
	eofPos = (MaxPage + 1) * MPageSize;
	srand(RS);
}

void FandTFile::WrPrefix()
{
	FandTFilePrefix T;

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
	case T00Format: {
		int RS = 0;
		unsigned short n = 0;
		unsigned short i = 0;
		memset(&T, '@', sizeof(T));
		std::string Pw = PwCode + Pw2Code;
		Pw = Coding::Code(Pw);
		RandSeed = RS;
		if (LicenseNr != 0) {
			for (i = 0; i < 20; i++) {
				Pw[i] = static_cast<char>(Random(255));
			}
		}
		n = 0x4000;
		T.Time = Random(100);
		if (Pw.length() != 40) {
			throw std::exception("Bad PwCode + Pw2Code length! Must be 40.");
		}
		memcpy(T.PwNew, Pw.c_str(), Pw.length());
		RandSeed = MLen + T.Time; // srand(MLen + T.Time);
		// for (i = 14; i < 511; i++) TX[i] = TX[i] ^ Random(255);
		RandIntByBytes(T.FreeRoot);
		RandIntByBytes(T.MaxPage);
		RandReal48ByBytes(T.TimeStmp);
		RandBooleanByBytes(T.HasCoproc);
		RandArrayByBytes(T.Rsrvd2, 25);
		RandArrayByBytes(T.Version, 4);
		RandArrayByBytes(T.LicText, 105);
		RandArrayByBytes(&T.Sum, 1);
		RandArrayByBytes(T.X1, 295);
		RandByteByBytes(T.LicNr);
		RandArrayByBytes(T.X2, 11);
		RandArrayByBytes(T.PwNew, 40);

		T.LicNr = LicenseNr;
		if (LicenseNr != 0) {
			unsigned char sum = 0;
			n = 0x6000;
			sum = T.LicNr;
			for (i = 0; i < 105; i++) sum += T.LicText[i];
			T.Sum = sum;
		}
		T.free_part = FreePart; // 8B
		T.rsrvd1 = Reserved; // 1B
		T.CompileProc = CompileProc; // 1B
		T.CompileAll = CompileAll; // 1B
		T.IRec = IRec; // 2B
		T.FreeRoot = FreeRoot; // 4B
		T.MaxPage = MaxPage; // 4B
		T.TimeStmp = TimeStmp; // 6B Pascal

		T.old_max_page = 0xffff;
		T.signum = 1;
		T.IRec += n;
		memcpy(T.Version, Version, 4);
		T.HasCoproc = HasCoproc;
		RandSeed = RS;
		break;
	}
	default:;
	}

	unsigned char header512[512]{ 0 };
	T.Save(header512);
	WriteData(0, 512, header512);
}

void FandTFile::SetEmpty()
{
	unsigned char X[MPageSize];
	short* XL = (short*)&X;
	switch (Format) {
	case DbtFormat: {
		MaxPage = 0;
		WrPrefix();
		break;
	}
	case FptFormat: {
		FreePart = 8; FptFormatBlockSize = 64;
		WrPrefix();
		break;
	}
	case T00Format: {
		FreeRoot = 0; MaxPage = 1; FreePart = MPageSize; MLen = 2 * MPageSize;
		WrPrefix();
		memset(X, 0, MPageSize); //FillChar(X, MPageSize, 0); 
		*XL = -510;
		WriteData(MPageSize, MPageSize, X);
		break;
	}
	default: break;
	}
}

void FandTFile::Create(const std::string& path)
{
	Handle = OpenH(path, _isOverwriteFile, Exclusive);
	TestErr();
	IRec = 1; LicenseNr = 0;

	PwCode = "";
	PwCode = AddTrailChars(PwCode, '@', 20);
	PwCode = Coding::Code(PwCode);

	Pw2Code = "";
	Pw2Code = AddTrailChars(Pw2Code, '@', 20);
	Pw2Code = Coding::Code(Pw2Code);

	eofPos = 2 * MPageSize;

	SetEmpty();
}

std::string FandTFile::Read(int32_t pos)
{
	std::string s;
	pos -= LicenseNr;
	if (pos <= 0) {
		// OldTxt=-1 in RDB!
		// return empty string
	}
	else {
		switch (Format) {
		case T00Format: {
			if ((pos < MPageSize) || (pos >= MLen)) {
				Err(891, false);
			}
			else {
				unsigned short len; // length of data
				ReadData(pos, 2, &len);

				if (len > MaxLStrLen + 1) {
					// max length has been exceeded
					Err(891, false);
				}
				else if (len == MaxLStrLen + 1) {
					s = ReadLongBuffer(pos);
				}
				else {
					const std::unique_ptr<uint8_t[]> data = std::make_unique_for_overwrite<uint8_t[]>(len);
					ReadBuffer(pos + 2, len, data.get());
					s = std::string((char*)data.get(), len);
				}
			}
			break;
		}
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

uint32_t FandTFile::GetLength(int32_t pos)
{
	int32_t result;

	pos -= LicenseNr;
	if (pos <= 0) {
		// OldTxt=-1 in RDB!
		return 0;
	}

	unsigned short len; // length of data
	ReadData(pos, 2, &len);

	if (len > MaxLStrLen + 1) {
		// max length has been exceeded
		result = 0;
		Err(891, false);
		
	}
	else if (len == MaxLStrLen + 1) {
		result = ReadLongBufferLength(pos);
	}
	else {
		result = len;
	}

	return result;
}

uint32_t FandTFile::Store(const std::string& data)
{
	uint8_t* s = (uint8_t*)data.c_str();
	size_t l = data.length();

	uint32_t pos = 0;
	char X[MPageSize + 1]{ 0 };
	struct stFptD { int Typ = 0, Len = 0; } FptD;

	if (l == 0) {
		return pos;
	}

	SetUpdateFlag(); //SetUpdHandle(Handle);

	switch (Format) {
	case T00Format: {
		if (l > MaxLStrLen) {
			// >65000B
			pos = NewPage(false);
			WriteLongBuffer(pos, l, s);
		}
		else if (l > MPageSize - 2) {
			// <65000B
			pos = NewPage(false);
			uint16_t data_len = (uint16_t)l;
			WriteData(pos, 2, &data_len);
			WriteBuffer(pos + 2, data_len, s);
		}
		else {
			// short text
			pos = PreparePositionForShortText(l);
			uint16_t data_len = (uint16_t)l;
			WriteData(pos, 2, &data_len);
			WriteBuffer(pos + 2, data_len, s);
		}

		break;
	}
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


void FandTFile::Delete(int32_t pos)
{
	if (pos <= 0) return;

	if ((Format != T00Format) || NotCached()) return;

	if ((pos < MPageSize) || (pos >= MLen)) {
		Err(889, false);
		return;
	}

	SetUpdateFlag(); //SetUpdHandle(Handle);

	if (pos < MPageSize || pos >= eofPos)
		return;								// mimo datovou oblast souboru

	uint8_t X[MPageSize]{ 0 };
	int32_t page_pos = pos & (0xFFFFFFFF << MPageShft);
	int32_t PosI = pos & (MPageSize - 1);

	ReadData(page_pos, MPageSize, X);
	void* wp = (unsigned short*)&X[PosI];
	int32_t l = *(unsigned short*)wp;

	if (l <= MPageSize - 2) {
		// small text on 1 page
		*(short*)&X[PosI] = (short)(-l);
		int32_t N = 0;
		wp = (short*)X;

		while (N < MPageSize - 2) {
			if (*(short*)wp > 0) {
				memset(&X[PosI + 2], 0, l);
				WriteData(page_pos, MPageSize, X);
				return;
			}
			N = N - *(short*)wp + 2;
			wp = (short*)&X[N];
		}

		if ((FreePart >= page_pos) && (FreePart < page_pos + MPageSize)) {
			memset(X, 0, MPageSize);
			*(short*)X = -510;
			FreePart = page_pos;
			WriteData(page_pos, MPageSize, X);
		}
		else {
			ReleasePage(page_pos);
		}
	}
	else {
		// long text on more than 1 page
		if (PosI != 0) {
			Err(889, false);
			return;
		}
	label2:
		l = *(unsigned short*)X;
		if (l > MaxLStrLen + 1) {
			Err(889, false);
			return;
		}
		const bool IsLongTxt = (l == MaxLStrLen + 1);
		l += 2;
	label4:
		ReleasePage(page_pos);
		if ((l > MPageSize) || IsLongTxt) {
			page_pos = *(__int32*)&X[MPageSize - 4];

			if ((page_pos < MPageSize) || (page_pos + MPageSize > MLen)) {
				Err(888, false);
				return;
			}
			ReadData(page_pos, MPageSize, X);
			if (l <= MPageSize) {
				goto label2;
			}
			l = l - (MPageSize - 4);
			goto label4;
		}
	}
}

void FandTFile::CloseFile()
{
	if (Handle != nullptr) {
		CloseClearH(&Handle);
		if (HandleError == 0) {
			Handle = nullptr;
			ClearUpdateFlag();
		}
		if ((!_parent->IsShared()) && (_parent->NRecs == 0) && (_parent->file_type != FileType::DBF)) {
			SetPathAndVolume(_parent->GetFileD());
			CPath = CExtToT(this, CDir, CName, CExt);
			MyDeleteFile(CPath);
		}
	}
}

uint32_t FandTFile::ReadBuffer(size_t position, size_t count, uint8_t* buffer)
{
	// position is after 2B length
	uint16_t L = 0;
	int32_t NxtPg = 0;
	int32_t offset = 0;
	uint16_t rest = MPageSize - (position & (MPageSize - 1));

	while (count > rest) {
		L = rest - 4;
		ReadData(position, L, &buffer[offset]);
		offset += L;
		count -= L;
		ReadData(position + L, 4, &NxtPg);
		position = NxtPg;

		if ((position < MPageSize) || (position + MPageSize > MLen)) {
			Err(890, false);
			memset(&buffer[offset], ' ', count);
			return 0;
		}
		rest = MPageSize;
	}

	// read last page
	ReadData(position, count, &buffer[offset]);
	return position + count;
}

std::string FandTFile::ReadLongBuffer(uint32_t position)
{
	// position is on the beginning of page

	std::string result;
	uint8_t buffer[MaxLStrLen]{ 0 };

	// read first 65000 bytes long block
	uint32_t next_read_pos = ReadBuffer(position + 2, MaxLStrLen, buffer);
	result += std::string((char*)buffer, MaxLStrLen);

	// read next blocks
	while (true) {
		// is there enough space to read address of next block?
		uint16_t rest = MPageSize - (next_read_pos & (MPageSize - 1));
		if (rest >= 4) {
			ReadData(next_read_pos, 4, &next_read_pos);
		}
		else {
			throw("FandTFile::ReadLongBuffer: there is not enough space to read address");
		}

		// get length of next section
		uint16_t data_len;
		ReadData(next_read_pos, 2, &data_len);

		if (data_len == MaxLStrLen + 1) {
			next_read_pos = ReadBuffer(next_read_pos + 2, MaxLStrLen, buffer);
			result += std::string((char*)buffer, MaxLStrLen);
		}
		else {
			// this is the last segment
			next_read_pos = ReadBuffer(next_read_pos + 2, data_len, buffer);
			result += std::string((char*)buffer, data_len);
			break;
		}
	}

	return result;
}

uint32_t FandTFile::ReadLongBufferLength(uint32_t position)
{
	// position is on the beginning of page
	uint32_t page_pos = position;
	uint32_t result = 0;

	// read next blocks
	while (true) {
		// get length of next section
		uint16_t data_len;
		ReadData(page_pos, 2, &data_len);

		if (data_len == MaxLStrLen + 1) {
			data_len = MaxLStrLen;
			uint16_t to_read = MaxLStrLen + 2; // first page is 2B smaller because contains length

			while (to_read > MPageSize - 4) {
				// read next page position (last 4B of the page)
				ReadData(page_pos + MPageSize - 4, 4, &page_pos);
				if ((page_pos < MPageSize) || (page_pos + MPageSize > MLen)) {
					Err(890, false);
					return 0;
				}
				to_read -= MPageSize - 4;
			}

			// points to the last page -> read next segment position
			ReadData(page_pos + to_read, 4, &page_pos);
			result += data_len; // +65000
		}
		else {
			// this is the last segment
			result += data_len;
			break;
		}
	}

	return result;
}

uint32_t FandTFile::WriteBuffer(size_t position, size_t count, uint8_t* buffer)
{
	// position is after 2B length
	uint16_t L = 0;
	int32_t NxtPg = 0;
	int32_t offset = 0;
	uint16_t Rest = MPageSize - (position & (MPageSize - 1));

	while (count > Rest) {
		L = Rest - 4;  // 4B next page
		WriteData(position, L, &buffer[offset]);
		offset += L;
		count -= L;
		NxtPg = NewPage(false);
		WriteData(position + L, 4, &NxtPg);
		position = NxtPg;
		Rest = MPageSize;
	}

	// write last page
	WriteData(position, count, &buffer[offset]);
	return position + count;
}

void FandTFile::WriteLongBuffer(size_t position, size_t count, uint8_t* buffer)
{
	// position is on the beginning of page
	const uint16_t L_LEN = MaxLStrLen + 1;
	uint32_t next_page_pos = 0;

	// write first 65000 bytes long block
	size_t offset = 0;
	WriteData(position, 2, (void*)&L_LEN);
	uint32_t next_free_space = WriteBuffer(position + 2, MaxLStrLen, &buffer[offset]);
	offset += MaxLStrLen;

	// write next blocks
	while (offset < count) {
		// is there enough space to write address of next block?
		uint16_t rest = MPageSize - (next_free_space & (MPageSize - 1));
		if (rest >= 4) {
			next_page_pos = NewPage(false);
			WriteData(next_free_space, 4, &next_page_pos);
		}
		else {
			throw("FandTFile::WriteLongBuffer: there is not enough space to write address");
		}

		size_t remains = count - offset;
		if (remains > MaxLStrLen) {
			WriteData(next_page_pos, 2, (void*)&L_LEN);
		}
		else {
			uint16_t last_length = (uint16_t)remains;
			WriteData(next_page_pos, 2, &last_length);
		}

		next_free_space = WriteBuffer(next_page_pos + 2, remains, &buffer[offset]);
		offset += remains;
	}
}

int32_t FandTFile::PreparePositionForShortText(size_t l)
{
	int32_t pos;
	int rest = MPageSize - FreePart % MPageSize;

	if (l + 2 <= rest) {
		pos = FreePart;
	}
	else {
		pos = NewPage(false);
		FreePart = pos;
		rest = MPageSize;
	}

	if (l + 4 >= rest) {
		FreePart = NewPage(false);
	}
	else {
		FreePart += l + 2;
		rest = l + 4 - rest;
		WriteData(FreePart, 2, &rest);
	}

	return pos;
}

int FandTFile::NewPage(bool NegL)
{
	int PosPg;
	unsigned char X[MPageSize]{ 0 };
	int* L = (int*)&X;

	if (FreeRoot != 0) {
		PosPg = FreeRoot << MPageShft;
		ReadData(PosPg, 4, &FreeRoot);

		if (FreeRoot > MaxPage) {
			Err(888, false);
			FreeRoot = 0;
			MaxPage++;
			MLen += MPageSize;
			PosPg = MaxPage << MPageShft;
			eofPos += MPageSize;
		}
	}
	else {
		MaxPage++;
		MLen += MPageSize;
		PosPg = MaxPage << MPageShft;
		eofPos += MPageSize;
	}

	if (NegL) {
		*L = -510;
	}
	WriteData(PosPg, MPageSize, X);
	return PosPg;
}

void FandTFile::ReleasePage(int PosPg)
{
	unsigned char X[MPageSize]{ 0 };
	*(int32_t*)X = FreeRoot;
	WriteData(PosPg, MPageSize, X);
	FreeRoot = PosPg >> MPageShft;
}

void FandTFile::GetMLen()
{
	MLen = (MaxPage + 1) << MPageShft;
}

void FandTFile::RandIntByBytes(int& nr)
{
	unsigned char* byte = (unsigned char*)&nr;
	for (size_t i = 0; i < 4; i++) {
		byte[i] = byte[i] ^ static_cast<BYTE>(Random(255));
	}
}

void FandTFile::RandByteByBytes(unsigned short& nr)
{
	unsigned char* byte = (unsigned char*)&nr;
	for (size_t i = 0; i < 2; i++) {
		byte[i] = byte[i] ^ static_cast<BYTE>(Random(255));
	}
}

void FandTFile::RandReal48ByBytes(double& nr)
{
	unsigned char* byte = (unsigned char*)&nr;
	for (size_t i = 0; i < 6; i++) {
		byte[i] = byte[i] ^ static_cast<BYTE>(Random(255));
	}
}

void FandTFile::RandBooleanByBytes(bool& nr)
{
	unsigned char* byte = (unsigned char*)&nr;
	for (size_t i = 0; i < sizeof(nr); i++) {
		byte[i] = byte[i] ^ static_cast<BYTE>(Random(255));
	}
}

void FandTFile::RandArrayByBytes(void* arr, size_t len)
{
	unsigned char* byte = (unsigned char*)arr;
	for (size_t i = 0; i < len; i++) {
		byte[i] = byte[i] ^ static_cast<BYTE>(Random(255));
	}
}

void WrDBaseHd()
{

}

