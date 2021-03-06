#include "XItemLeaf.h"

XItemLeaf::XItemLeaf(BYTE* data)
{
	RecNr = *(int*)data & 0x00FFFFFF;
	M = data[3];
	L = data[4];
	this->data = new BYTE[L];
	memcpy(this->data, &data[5], L);
}

XItemLeaf::XItemLeaf(const XItemLeaf& orig)
{
	RecNr = orig.RecNr;
	M = orig.M;
	L = orig.L;
	this->data = new BYTE[L];
	memcpy(this->data, orig.data, L);
}

XItemLeaf::XItemLeaf(unsigned RecNr, BYTE M, BYTE L, pstring& s)
{
	this->RecNr = RecNr;
	this->M = M;
	this->L = L;
	this->data = new BYTE[L];
	memcpy(this->data, &s[1 + M], L);
}

XItemLeaf::~XItemLeaf()
{
	delete[] data;
	data = nullptr;
}

size_t XItemLeaf::size()
{
	return 3 + 2 + L; // 3 cislo zaznamu, 2 L+M, delka zaznamu
}

size_t XItemLeaf::dataLen()
{
	return L;
}

size_t XItemLeaf::Serialize(BYTE* buffer, size_t bufferSize)
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
