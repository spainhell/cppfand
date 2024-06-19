#pragma once
#include <string>
#include <vector>
#include "../Common/pstring.h"

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
	void StoreKF(FileD* file_d, KeyFldD* KF, void* record);
	void PackKF(FileD* file_d, KeyFldD* KF, void* record);
	void PackKF(FileD* file_d, std::vector<KeyFldD*>& KF, void* record);
	bool PackFrml(FileD* file_d, std::vector<FrmlElem*>& FL, KeyFldD* KF, void* record);
#ifdef FandSQL
	void GetF(unsigned short Off, unsigned short Len, bool Descend, void* Buf);
	void GetD(unsigned short Off, bool Descend, void* rdb);
	void GetN(unsigned short Off, unsigned short Len, bool Descend, void* Buf);
	unsigned short GetA(unsigned short Off, unsigned short Len, bool CompLex, bool Descend, void* Buf);
#endif
private:
	void StoreD(void* R, bool descend);
	void StoreN(void* N, unsigned short len, bool descend);
	void StoreF(void* F, unsigned short len, bool descend);
	void StoreA(void* A, unsigned short len, bool compLex, bool descend);
	void negate_esdi(void* data, size_t len);
};
