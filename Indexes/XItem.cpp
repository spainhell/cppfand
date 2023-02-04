#include "XItem.h"

WORD XItem::GetM()
{
	return M;
}

void XItem::PutM(WORD M)
{
	this->M = M;
}

WORD XItem::GetL()
{
	return L;
}

void XItem::PutL(WORD L)
{
	this->L = L;
}

std::string XItem::GetKey(std::string& previous_key)
{
	const std::string part1 = previous_key.substr(0, M);
	const std::string part2 = std::string(reinterpret_cast<char*>(data), L);
	return part1 + part2;
}

size_t XItem::data_len()
{
	return L;
}
