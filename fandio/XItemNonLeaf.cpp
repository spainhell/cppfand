#include "XItemNonLeaf.h"

XItemNonLeaf::XItemNonLeaf(unsigned char* data)
{
	RecordsCount = *(int*)data & 0x00FFFFFF;
	DownPage = *(uint32_t*)&data[3];
	M = data[7];
	L = data[8];
	this->data = new unsigned char[L];
	memcpy(this->data, &data[9], L);
}

XItemNonLeaf::XItemNonLeaf(const XItemNonLeaf& orig)
{
	RecordsCount = orig.RecordsCount;
	DownPage = orig.DownPage;
	M = orig.M;
	L = orig.L;
	this->data = new unsigned char[L];
	memcpy(this->data, orig.data, L);
}

XItemNonLeaf::XItemNonLeaf(unsigned recordsCount, unsigned downPage, unsigned char M, unsigned char L, std::string& s)
{
	this->RecordsCount = recordsCount;
	this->DownPage = downPage;
	this->M = M;
	this->L = L;
	this->data = new unsigned char[L];
	memcpy(this->data, &s.c_str()[M], L);
}

XItemNonLeaf::~XItemNonLeaf()
{
	delete[] data;
	data = nullptr;
}

int XItemNonLeaf::GetN()
{
	return RecordsCount;
}

void XItemNonLeaf::PutN(int N)
{
	this->RecordsCount = N;
}

//XItem* XItemNonLeaf::Next()
//{
//	unsigned char recLen = data[0];
//	// dalsi zaznam zacina hned za daty o delce recLen
//	auto xi = new XItemNonLeaf(&data[recLen + 1]);
//	return xi;
//}

//size_t XItemNonLeaf::UpdStr(pstring* S)
//{
//	(*S)[0] = M + L; // nova delka retezce
//	memcpy(&(*S)[M + 1], data, L);
//	return L + 2;
//}

size_t XItemNonLeaf::size()
{
	return 3 + 4 + 2 + L; // 3 cislo zaznamu, 4 cislo stranky, 2 L+M, delka zaznamu
}

size_t XItemNonLeaf::Serialize(unsigned char* buffer, size_t bufferSize)
{
	if (bufferSize < size()) return -1;
	size_t offset = 0;
	memcpy(&buffer[offset], &RecordsCount, 3);
	offset += 3;
	memcpy(&buffer[offset], &DownPage, 4);
	offset += 4;
	buffer[offset++] = M;
	buffer[offset++] = L;
	memcpy(&buffer[offset], data, L);
	offset += L;
	memset(&buffer[offset], 0, bufferSize - offset);
	return offset; // pocet zapsanych Bytu
}
