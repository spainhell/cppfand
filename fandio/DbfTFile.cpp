#include "DbfTFile.h"

#include <vector>

#include "FileIO.h"
#include "Messages.h"
#include "FileD.h"
#include "DbfFile.h"
#include "FandTFile.h"
#include "../fandbase/constants.h"

// Memo files of .DBF (FILEACC.PAS, TFile for DbtFormat and FptFormat):
//   .DBT (dBase III): header = number of the next free 512 B page; a text is
//        stored from the beginning of a page and ends with two ^Z characters.
//   .FPT (FoxPro): header = next free block and block size (big-endian); a text
//        block starts with its type (1 = text) and length (both big-endian).
// Deleted texts are not reused (as in the original).

namespace
{
	const int FptHeaderSize = 512;

	uint32_t big_endian_32(const uint8_t* p)
	{
		return (uint32_t(p[0]) << 24) | (uint32_t(p[1]) << 16) | (uint32_t(p[2]) << 8) | uint32_t(p[3]);
	}

	uint16_t big_endian_16(const uint8_t* p)
	{
		return static_cast<uint16_t>((p[0] << 8) | p[1]);
	}

	void put_big_endian_32(uint8_t* p, uint32_t v)
	{
		p[0] = static_cast<uint8_t>(v >> 24); p[1] = static_cast<uint8_t>(v >> 16);
		p[2] = static_cast<uint8_t>(v >> 8); p[3] = static_cast<uint8_t>(v);
	}

	void put_big_endian_16(uint8_t* p, uint16_t v)
	{
		p[0] = static_cast<uint8_t>(v >> 8); p[1] = static_cast<uint8_t>(v);
	}
}

DbfTFile::DbfTFile(DbfFile* parent)
{
	_parent = parent;
}

DbfTFile::DbfTFile(const DbfTFile& orig, DbfFile* parent)
{
	_parent = parent;
	Handle = orig.Handle;
	Format = orig.Format;
	FreePart = orig.FreePart;
	FptFormatBlockSize = orig.FptFormatBlockSize;
	MaxPage = orig.MaxPage;
	MLen = orig.MLen;
	LicenseNr = orig.LicenseNr;
}

DbfTFile::~DbfTFile()
{
}

void DbfTFile::Err(unsigned short n, bool ex) const
{
	fandio::ShowFileMessage(_parent->GetFileD(), fandio::FilePart::Text, n);
	if (ex) {
		_parent->GetFileD()->Close();
		fandio::Abort(n);
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
		return int(MaxPage + 1) << MPageShft;
	}
}

void DbfTFile::RdPrefix(bool check)
{
	if (check && FileSizeH(Handle) <= MPageSize) {
		SetEmpty();
		return;
	}

	uint8_t header[MPageSize]{};
	ReadData(0, MPageSize, header);
	LicenseNr = 0;

	if (Format == DbtFormat) {
		const int32_t next_page = header[0] | (header[1] << 8) | (header[2] << 16) | (header[3] << 24);
		MaxPage = next_page - 1;
		GetMLen();
	}
	else {
		FreePart = static_cast<int>(big_endian_32(header));
		FptFormatBlockSize = big_endian_16(header + 6);
		if (FptFormatBlockSize == 0) FptFormatBlockSize = 64;
	}
}

void DbfTFile::WrPrefix()
{
	uint8_t header[MPageSize];

	if (Format == DbtFormat) {
		memset(header, ' ', sizeof(header));
		const int32_t next_page = MaxPage + 1;
		memcpy(header, &next_page, 4); // little-endian
		WriteData(0, MPageSize, header);
	}
	else {
		uint8_t fpt[FptHeaderSize]{};
		put_big_endian_32(fpt, static_cast<uint32_t>(FreePart));
		put_big_endian_16(fpt + 6, FptFormatBlockSize);
		WriteData(0, FptHeaderSize, fpt);
	}
}

void DbfTFile::SetEmpty()
{
	if (Format == DbtFormat) {
		MaxPage = 0;
	}
	else {
		FreePart = 8;
		FptFormatBlockSize = 64;
	}
	WrPrefix();
	SetUpdateFlag();
}

std::string DbfTFile::Read(int32_t pos)
{
	pos -= LicenseNr;
	if (pos <= 0) {
		return ""; // OldTxt=-1 in RDB!
	}

	if (Format == DbtFormat) {
		// pages until ^Z, at most 32 kB (as in the original)
		std::string s;
		std::vector<char> page(MPageSize);
		int32_t offset = pos << MPageShft;
		while (s.length() <= 32768 - MPageSize) {
			const size_t n = ReadData(offset, MPageSize, page.data());
			for (size_t i = 0; i < MPageSize; i++) {
				if (i >= n || page[i] == 0x1A) return s;
				s += page[i];
			}
			offset += MPageSize;
		}
		return s;
	}
	else {
		uint8_t block_header[8]{};
		const int32_t offset = pos * FptFormatBlockSize;
		ReadData(offset, sizeof(block_header), block_header);
		if (big_endian_32(block_header) != 1) {
			return ""; // not a text
		}
		const uint32_t length = big_endian_32(block_header + 4) & 0x7FFF;
		std::string s(length, '\0');
		if (length > 0) ReadData(offset + sizeof(block_header), length, s.data());
		return s;
	}
}

uint32_t DbfTFile::Store(const std::string& data)
{
	size_t l = data.length();
	if (l == 0) {
		return 0;
	}
	if (l > 0x7fff) l = 0x7fff;

	SetUpdateFlag();
	uint32_t pos;

	if (Format == DbtFormat) {
		pos = MaxPage + 1;
		const int32_t offset = pos << MPageShft;
		WriteData(offset, l, const_cast<char*>(data.data()));
		// the text ends with ^Z^Z, the rest of the last page is filled with spaces
		const size_t rest = MPageSize - (l + 2) % MPageSize;
		std::vector<char> tail(rest + 2, ' ');
		tail[0] = 0x1A; tail[1] = 0x1A;
		WriteData(offset + l, tail.size(), tail.data());
		MaxPage += static_cast<int32_t>((l + 2 + rest) / MPageSize);
	}
	else {
		pos = FreePart;
		int32_t offset = FreePart * FptFormatBlockSize;
		uint8_t block_header[8];
		put_big_endian_32(block_header, 1); // text
		put_big_endian_32(block_header + 4, static_cast<uint32_t>(l));
		FreePart = FreePart + static_cast<int>((sizeof(block_header) + l - 1) / FptFormatBlockSize) + 1;
		WriteData(offset, sizeof(block_header), block_header);
		offset += sizeof(block_header);
		WriteData(offset, l, const_cast<char*>(data.data()));
		offset += static_cast<int32_t>(l);
		const int32_t rest = FreePart * FptFormatBlockSize - offset;
		if (rest > 0) {
			std::vector<char> fill(rest, ' ');
			WriteData(offset, rest, fill.data());
		}
	}
	return pos;
}

void DbfTFile::Delete(int32_t pos)
{
	// texts of .DBF are not reused (as in the original)
}

void DbfTFile::Create(const std::string& path)
{
	Handle = OpenH(path, _isOverwriteFile, Exclusive);
	TestErr();
	LicenseNr = 0;
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
	MLen = (MaxPage + 1) << MPageShft;
}
