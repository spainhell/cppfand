#include "FandTFile.h"

#include <memory>

#include "FandTFilePrefix.h"
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
		if (ex) {
			_parent->GetFileD()->Close();
			GoExit(MsgLine);
		}
	}
}

void FandTFile::TestErr() const
{
	if (HandleError != 0) Err(700 + HandleError, true);
}

int FandTFile::UsedFileSize() const
{
	return int(MaxPage + 1) << MPageShft;
}

bool FandTFile::NotCached() const
{
	return !IsWork && _parent->GetFileD()->NotCached();
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

	//if (!IsWork) { CompileAll = true; }

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
		if (!IsWork && (_parent->file_type == FandFileType::RDB)) LicenseNr = T.LicNr;
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

	unsigned char header512[512]{ 0 };
	T.Save(header512);
	WriteData(0, 512, header512);
}

void FandTFile::SetEmpty()
{
	uint8_t X[MPageSize];
	int16_t* XL = (int16_t*)&X;

	FreeRoot = 0;
	MaxPage = 1;
	FreePart = MPageSize;
	MLen = 2 * MPageSize;

	WrPrefix();
	memset(X, 0, MPageSize);
	*XL = -510;
	WriteData(MPageSize, MPageSize, X);
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
				//auto check = GetLength(pos);
				s = ReadLongBuffer(pos);
			}
			else {
				const std::unique_ptr<uint8_t[]> data = std::make_unique_for_overwrite<uint8_t[]>(len);
				ReadBuffer(pos + 2, len, data.get());
				s = std::string((char*)data.get(), len);
			}
		}
	}

	return s;
}

uint32_t FandTFile::GetLength(int32_t pos)
{
	uint32_t result;

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

	if (l > MaxLStrLen) {
		// > 65000B
		pos = NewPage(false);
		WriteLongBuffer(pos, l, s);
		//auto check = GetLength(pos);
	}
	else if (l > MPageSize - 2) {
		// <= 65000B
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

	return pos;
}

void FandTFile::Delete(int32_t pos)
{
	if (pos <= 0) return;

	if (/*(Format != T00Format) ||*/ NotCached()) return;

	if ((pos < MPageSize) || (pos >= MLen)) {
		Err(889, false);
		return;
	}

	SetUpdateFlag();

	uint32_t page_pos;
	int16_t l;
	uint16_t u;

	if (pos < MPageSize || pos >= eofPos)
		return; // mimo datovou oblast souboru

	ReadData(pos, 2, &l); // delka stringu
	if (static_cast<unsigned short>(l) < MPageSize - 2) {
		// SHORT TEXT na sdilene strance
		char buffer[MPageSize];
		u = static_cast<unsigned short>(pos) % MPageSize; // offset ve strance
		page_pos = pos - u; // pozice stranky v souboru
		ReadData(page_pos, MPageSize, buffer); // nactu stranku
		*(short*)(buffer + u) = -l; // zaporne delku
		char* p = buffer;

		while (!false) {
			// spojuji volne fragmenty (posledni je vzdy volny!)

			// POZOR, fand v aktualni strance neudrzuje zbyvajici delku!!!, proto nasl. IF
			if (page_pos + p - buffer == FreePart) {
				// pred freePart platny string n. prazdny aktualni
				break;
			}

			l = *(short*)p;

			if (l <= 0) {
				// je-li fragment volny
				char* p1 = p - l + 2; // adresa nasledujiciho fragmentu

				if (page_pos + p1 - buffer == FreePart) {
					// volna cast aktualniho segmentu
					FreePart = page_pos + p - buffer; // pripojim to k volne casti
					break;
				}

				if (p1 >= buffer + MPageSize - 2) {
					// byl posledni (fragment je alespon 3)
					break;
				}

				if ((l = *(short*)p1) <= 0) {
					// je-li volny
					*(short*)p += l - 2; // pripojim ho k predchozimu
				}
				else {
					p = p1; // jinak vezmu nasledujici (nebo dalsi? + l + 2)
				}
			}
			else
				p += l + 2; // preskocim obsazeny fragment
		}

		if (*(short*)buffer <= -(MPageSize - 2) && page_pos != FreePart) {
			// jediny volny fragment ve strance
			// a neni to aktualni segment
			ReleasePage(page_pos); // uvolnim stranku
		}
		else {
			WriteData(page_pos, MPageSize, buffer); // zapis stranku (celou, mohlo se slucovat)
		}
	}
	else {
		// LONG TEXT
		while (true) {
			// cyklus uvolnovani segmentu
			u = static_cast<unsigned short>(l);
			if (u == MaxLStrLen + 1) // posledni segment?
				--u; // ano, smazu priznak
			u += 2; // do 1. zapocitam delku
			while (true) {
				// cyklus uvolnovani stranek v segmentu
				ReadData(pos + MPageSize - 4, 4, &page_pos); // nactu adresu dalsi stranky
				ReleasePage(pos);
				pos = page_pos;
				if (u <= MPageSize) // posledni stranka v segmentu
					break;
				u -= MPageSize - 4;
			}
			if (static_cast<unsigned short>(l) != MaxLStrLen + 1) {
				// dalsi segment?
				break; // ne
			}
			ReadData(pos, 2, &l); // delka segmentu
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
		if ((!_parent->IsShared()) && (_parent->NRecs == 0)) {
			_parent->GetFileD()->SetPathAndVolume();
			CPath = _parent->GetFileD()->CExtToT(CDir, CName, CExt);
			MyDeleteFile(CPath);
		}
	}
}

/// <summary>
/// 
/// </summary>
/// <param name="position"></param>
/// <param name="count"></param>
/// <param name="buffer"></param>
/// <returns>Returns position of last read page</returns>
uint32_t FandTFile::ReadBuffer(size_t position, size_t count, uint8_t* buffer)
{
	// position is after 2B length
	uint16_t L = 0;
	int32_t next_page_pos = 0;
	int32_t offset = 0;
	uint16_t rest = MPageSize - (position & (MPageSize - 1));

	while (count > rest) {
		L = rest - 4;
		ReadData(position, L, &buffer[offset]);
		offset += L;
		count -= L;
		ReadData(position + L, 4, &next_page_pos);
		position = next_page_pos;

		if ((position < MPageSize) || (position + MPageSize > MLen)) {
			Err(890, false);
			memset(&buffer[offset], ' ', count);
			return 0;
		}
		rest = MPageSize;
	}

	// read last page
	ReadData(position, count, &buffer[offset]);
	return next_page_pos;
}

std::string FandTFile::ReadLongBuffer(uint32_t position)
{
	// position is on the beginning of page

	std::string result;
	uint8_t buffer[MaxLStrLen]{ 0 };

	// read first 65000 bytes long block
	uint32_t last_read_page_pos = ReadBuffer(position + 2, MaxLStrLen, buffer);
	result += std::string((char*)buffer, MaxLStrLen);

	// read next blocks
	while (true) {
		ReadData(last_read_page_pos + MPageSize - 4, 4, &last_read_page_pos);

		// get length of next section
		uint16_t data_len;
		ReadData(last_read_page_pos, 2, &data_len);

		if (data_len == MaxLStrLen + 1) {
			last_read_page_pos = ReadBuffer(last_read_page_pos + 2, MaxLStrLen, buffer);
			result += std::string((char*)buffer, MaxLStrLen);
		}
		else {
			// this is the last segment
			last_read_page_pos = ReadBuffer(last_read_page_pos + 2, data_len, buffer);
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
				if ((page_pos < MPageSize) || (page_pos + MPageSize > MLen)) {
					Err(890, false);
					return 0;
				}
				// read next page position (last 4B of the page)
				ReadData(page_pos + MPageSize - 4, 4, &page_pos);
				to_read -= MPageSize - 4;
			}

			// points to the last page -> read next segment position
			ReadData(page_pos + MPageSize - 4, 4, &page_pos);
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

/// <summary>
/// 
/// </summary>
/// <param name="position"></param>
/// <param name="count"></param>
/// <param name="buffer"></param>
/// <returns>Last written page position</returns>
uint32_t FandTFile::WriteBuffer(size_t position, size_t count, uint8_t* buffer)
{
	// position is after 2B length
	uint16_t L = 0;
	int32_t next_page_pos = 0;
	int32_t offset = 0;
	uint16_t Rest = MPageSize - (position & (MPageSize - 1));

	while (count > Rest) {
		L = Rest - 4;  // 4B next page
		WriteData(position, L, &buffer[offset]);
		offset += L;
		count -= L;
		next_page_pos = NewPage(false);
		WriteData(position + L, 4, &next_page_pos);
		position = next_page_pos;
		Rest = MPageSize;
	}

	// write last page
	WriteData(position, count, &buffer[offset]);
	return next_page_pos;
}

void FandTFile::WriteLongBuffer(size_t position, size_t count, uint8_t* buffer)
{
	// position is on the beginning of page
	const uint16_t L_LEN = MaxLStrLen + 1;

	// write first 65000 bytes long block
	size_t offset = 0;
	WriteData(position, 2, (void*)&L_LEN);
	uint32_t last_written_page_pos = WriteBuffer(position + 2, MaxLStrLen, &buffer[offset]);
	offset += MaxLStrLen;

	// write next blocks
	while (offset < count) {
		uint32_t next_page_pos = NewPage(false);

		// write next segment position to the end of last written page
		WriteData(last_written_page_pos + MPageSize - 4, 4, &next_page_pos);

		size_t remains = count - offset;
		if (remains > MaxLStrLen) {
			WriteData(next_page_pos, 2, (void*)&L_LEN);
		}
		else {
			uint16_t last_length = (uint16_t)remains;
			WriteData(next_page_pos, 2, &last_length);
		}

		last_written_page_pos = WriteBuffer(next_page_pos + 2, remains, &buffer[offset]);
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

void FandTFile::ReleasePage(int page_pos)
{
	unsigned char X[MPageSize]{ 0 };
	*(int32_t*)X = FreeRoot;
	WriteData(page_pos, MPageSize, X);
	FreeRoot = page_pos >> MPageShft;


	//DWORD error;

	//if (page_pos == (eofPos - MPageSize)) {
	//	// je-li to posledni stranka souboru
	//	eofPos = page_pos;
	//	TruncF(Handle, error, eofPos); // tak zkratim soubor
	//}
	//else {
	//	long l = FreeRoot / MPageSize;
	//	WriteData(page_pos, 4, &l); // uvolnovanou stranku dam do seznamu volnych
	//	FreeRoot = page_pos;
	//}
}

void FandTFile::GetMLen()
{
	MLen = (MaxPage + 1) << MPageShft;
}

void FandTFile::RandIntByBytes(int& nr)
{
	unsigned char* byte = (unsigned char*)&nr;
	for (size_t i = 0; i < 4; i++) {
		byte[i] = byte[i] ^ static_cast<uint8_t>(Random(255));
	}
}

void FandTFile::RandByteByBytes(unsigned short& nr)
{
	unsigned char* byte = (unsigned char*)&nr;
	for (size_t i = 0; i < 2; i++) {
		byte[i] = byte[i] ^ static_cast<uint8_t>(Random(255));
	}
}

void FandTFile::RandReal48ByBytes(double& nr)
{
	unsigned char* byte = (unsigned char*)&nr;
	for (size_t i = 0; i < 6; i++) {
		byte[i] = byte[i] ^ static_cast<uint8_t>(Random(255));
	}
}

void FandTFile::RandBooleanByBytes(bool& nr)
{
	unsigned char* byte = (unsigned char*)&nr;
	for (size_t i = 0; i < sizeof(nr); i++) {
		byte[i] = byte[i] ^ static_cast<uint8_t>(Random(255));
	}
}

void FandTFile::RandArrayByBytes(void* arr, size_t len)
{
	unsigned char* byte = (unsigned char*)arr;
	for (size_t i = 0; i < len; i++) {
		byte[i] = byte[i] ^ static_cast<uint8_t>(Random(255));
	}
}

void WrDBaseHd()
{

}

