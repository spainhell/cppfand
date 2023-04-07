#pragma once

class LongStr
{
public:
	LongStr() { A = new char[50] { 0 }; allocated = 50; LL = 0; }
	LongStr(size_t size) {
		if (size == 0) size = 50;
		A = new char[size] {0};
		allocated = size;
		LL = 0;
	}
	LongStr(char* data, size_t size) {
		A = data;
		allocated = size;
		LL = size;
	}
	~LongStr() { delete[] A; }
	size_t LL;
	char* A;
private:
	size_t allocated = 0;
};