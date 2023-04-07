#include "XItemLeaf.h"

XItemLeaf::XItemLeaf(unsigned char* data)
{
	RecNr = *(int*)data & 0x00FFFFFF;
	M = data[3];
	L = data[4];
	this->data = new unsigned char[L];
	memcpy(this->data, &data[5], L);
}

XItemLeaf::XItemLeaf(const XItemLeaf& orig)
{
	RecNr = orig.RecNr;
	M = orig.M;
	L = orig.L;
	this->data = new unsigned char[L];
	memcpy(this->data, orig.data, L);
}

XItemLeaf::XItemLeaf(unsigned int RecNr, unsigned char M, unsigned char L, pstring& s)
{
	this->RecNr = RecNr;
	this->M = M;
	this->L = L;
	this->data = new unsigned char[L];
	memcpy(this->data, &s[1 + M], L);

#if _DEBUG
	this->key = s;
#endif
}

XItemLeaf::XItemLeaf(unsigned RecNr, unsigned char M, unsigned char L, std::string& s)
{
	this->RecNr = RecNr;
	this->M = M;
	this->L = L;
	this->data = new unsigned char[L];
	memcpy(this->data, &s.c_str()[M], L);

#if _DEBUG
	this->key = s;
#endif
}

XItemLeaf::~XItemLeaf()
{
	delete[] data;
	data = nullptr;
}

int XItemLeaf::GetN()
{
	return RecNr;
}

void XItemLeaf::PutN(int N)
{
	this->RecNr = N;
}

//XItem* XItemLeaf::Next()
//{
//	unsigned char recLen = data[0];
//	// dalsi zaznam zacina hned za daty o delce recLen
//	auto xi = new XItemLeaf(&data[recLen + 1]);
//	return xi;
//}

//size_t XItemLeaf::UpdStr(pstring* S)
//{
//	(*S)[0] = M + L; // nova delka retezce
//	memcpy(&(*S)[M + 1], data, L);
//	return L + 2;
//}

size_t XItemLeaf::size()
{
	return 3 + 2 + L; // 3 cislo zaznamu, 2 L+M, delka zaznamu
}

size_t XItemLeaf::Serialize(unsigned char* buffer, size_t bufferSize)
{
	if (bufferSize < size()) return -1;
	size_t offset = 0;
	memcpy(&buffer[offset], &RecNr, 3);
	offset += 3;
	buffer[offset++] = M;
	buffer[offset++] = L;
	memcpy(&buffer[offset], data, L);
	offset += L;
	memset(&buffer[offset], 0, bufferSize - offset);
	return offset; // pocet zapsanych Bytu
}
