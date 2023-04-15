#include "ResFile.h"

void ResFile::Open(std::string path)
{
	//CPath = FandResName;
	//CVol = "";
	DWORD error;
	Handle = OpenF(path, error, GENERIC_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL);
	FullName = path;
	if (error != 0) {
		printf("can't open %s\n", path.c_str());
		system("pause");
		exit(-1);
	}
}

WORD ResFile::Get(WORD Kod, void** P)
{
	WORD l = A[Kod].Size;
	*P = new BYTE[l];
	long sizeF = SizeF(Handle, error);
	long seekF = SeekF(Handle, error, A[Kod - 1].Pos);
	size_t readF = ReadF(Handle, *P, l, error);
	return l;
}

std::string ResFile::Get(WORD Kod)
{
	char* tmpCh = new char[A[Kod].Size];
	WORD l = A[Kod].Size;

	long sizeF = SizeF(Handle, error);
	long seekF = SeekF(Handle, error, A[Kod].Pos);
	size_t readF = ReadF(Handle, tmpCh, l, error);
	std::string result = std::string(tmpCh, l);
	delete[] tmpCh;
	return result;
}

LongStr* ResFile::GetStr(WORD Kod)
{
	LongStr* s = new LongStr(A[Kod].Size + 2);
	s->LL = A[Kod].Size;
	SeekF(Handle, error, A[Kod].Pos);
	ReadF(Handle, s->A, A[Kod].Size, error);
	return s;
}

bool ResFile::ReadMessage(int msg_nr, std::string& message)
{
	bool result = true;
	WORD j = 0;
	WORD o = 0;
	bool found = false;
	for (int i = 0; i < MsgIdxN; i++) {
		WORD Nr = MsgIdx[i].Nr;
		BYTE Count = MsgIdx[i].Count;
		WORD Ofs = MsgIdx[i].Ofs;
		if (msg_nr >= Nr && msg_nr < Nr + Count) {
			j = msg_nr - Nr + 1;
			o = Ofs;
			found = true;
			break;
		}
	}
	if (!found) {
		o = 0;
		j = 1;
		return false;
	}
	message = get_message(FrstMsgPos + o, j);
	return result;
}

std::string ResFile::get_message(int offset, WORD order)
{
	SeekF(Handle, error, offset, 0);

	BYTE length = 0;
	char* buffer = nullptr;

	for (int i = 1; i <= order; i++) {
		ReadF(Handle, &length, 1, error); // read length only (1B)
		if (i == order) {
			buffer = new char[length];
			ReadF(Handle, buffer, length, error);
		}
		else {
			SeekF(Handle, error, length, 1);
		}
	}
	std::string result = std::string(buffer, length);
	delete[] buffer; buffer = nullptr;
	return result;
}

void ResFile::ReadInfo()
{
	WORD version;
	ReadF(Handle, &version, 2, error);
	if (version != ResVersion) {
		printf("FAND.RES incorr. version\n");
		system("pause");
		exit(-1);
	}

	for (int i = 0; i < FandFace; i++) {
		ReadF(Handle, &A[i].Pos, sizeof(A->Pos), error);
		ReadF(Handle, &A[i].Size, sizeof(A->Size), error);
	}

	// NACTENI INFORMACI O ZPRAVACH Z FAND.RES
	ReadF(Handle, &MsgIdxN, 2, error);
	for (int ii = 0; ii < MsgIdxN; ii++) {
		TMsgIdxItem newItem{ 0, 0, 0 };
		ReadF(Handle, &newItem.Nr, sizeof(newItem.Nr), error);       // WORD
		ReadF(Handle, &newItem.Ofs, sizeof(newItem.Ofs), error);     // WORD
		ReadF(Handle, &newItem.Count, sizeof(newItem.Count), error); // BYTE
		MsgIdx.push_back(newItem);
	}
	FrstMsgPos = PosF(Handle, error);
}
