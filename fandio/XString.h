#pragma once
#include <string>
#include <vector>
#include "../Common/pstring.h"

class Record;
class FrmlElem;
class KeyFldD;
class FileD;

class XString
{
public:
	pstring S;
	void Clear();
	void StoreReal(double R, KeyFldD* KF);
	void StoreStr(std::string V, KeyFldD* KF);
	void StoreBool(bool B, KeyFldD* KF);
	void StoreKF(KeyFldD* KF, Record* record);
	void PackKF(std::vector<KeyFldD*>& KF, Record* record);
	bool PackFrml(FileD* file_d, std::vector<FrmlElem*>& FL, std::vector<KeyFldD*>& KF, Record* record);
#ifdef FandSQL
	void GetF(unsigned short Off, unsigned short Len, bool Descend, void* Buf);
	void GetD(unsigned short Off, bool Descend, void* rdb);
	void GetN(unsigned short Off, unsigned short Len, bool Descend, void* Buf);
	unsigned short GetA(unsigned short Off, unsigned short Len, bool CompLex, bool Descend, void* Buf);
#endif
private:
	void StoreD(uint8_t* R, bool descend);
	void StoreN(uint8_t* N, unsigned short len, bool descend);
	void StoreF(uint8_t* F, unsigned short len, bool descend);
	void StoreA(uint8_t* A, unsigned short len, bool compLex, bool descend);
	void negate_esdi(uint8_t* data, size_t len);
};
