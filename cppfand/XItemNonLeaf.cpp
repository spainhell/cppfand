#include "XItemNonLeaf.h"

XItemNonLeaf::XItemNonLeaf(BYTE* data)
{
	RecNr = *(int*)data & 0x00FFFFFF;
	DownPage = *(uint32_t*)&data[3];
	M = data[7];
	L = data[8];
	this->data = new BYTE[L];
	memcpy(this->data, &data[9], L);
}

XItemNonLeaf::XItemNonLeaf(const XItemNonLeaf& orig)
{
	RecNr = orig.RecNr;
	DownPage = orig.DownPage;
	M = orig.M;
	L = orig.L;
	this->data = new BYTE[L];
	memcpy(this->data, orig.data, L);
}

XItemNonLeaf::~XItemNonLeaf()
{
	delete[] data;
	data = nullptr;
}

size_t XItemNonLeaf::size()
{
	return 3 + 4 + 2 + L; // 3 cislo zaznamu, 4 cislo stranky, 2 L+M, delka zaznamu
}

size_t XItemNonLeaf::data_len()
{
	return L;
}

size_t XItemNonLeaf::Serialize(BYTE* buffer, size_t bufferSize)
{
	if (bufferSize < size()) return -1;
	size_t offset = 0;
	memcpy(&buffer[offset], &RecNr, 3);
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
