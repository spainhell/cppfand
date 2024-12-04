#include "DBaseField.h"

uint8_t* DBaseField::GetData()
{
	memset(data_, 0, DBaseFieldSize);
	memcpy(data_, name.c_str(), name.length());
	memcpy(data_ + 11, &typ, 1);
	memcpy(data_ + 12, &displ, 4);
	memcpy(data_ + 16, &len, 1);
	memcpy(data_ + 17, &dec, 1);
	return data_;
}

size_t DBaseField::GetDataLength()
{
	return DBaseFieldSize;
}
