#pragma once
#include <vector>

#include "../cppfand/constants.h"
#include "../cppfand/pstring.h"

class KeyFldD;
struct FrmlListEl;

class XString // ø. 254
{
public:
	pstring S; // S:string255;
	//BYTE S[256]{ 0 };
	void Clear(); // index.pas ASM
	void StoreReal(double R, KeyFldD* KF);
	void StoreStr(std::string V, KeyFldD* KF);
	void StoreBool(bool B, KeyFldD* KF);
	void StoreKF(KeyFldD* KF);
	void PackKF(KeyFldD* KF);
	void PackKF(std::vector<KeyFldD*>& KF);
	bool PackFrml(FrmlListEl* FL, KeyFldD* KF);
#ifdef FandSQL
	void GetF(WORD Off, WORD Len, bool Descend, void* Buf);
	void GetD(WORD Off, bool Descend, void* R);
	void GetN(WORD Off, WORD Len, bool Descend, void* Buf);
	WORD GetA(WORD Off, WORD Len, bool CompLex, bool Descend, void* Buf);
#endif
private:
	void StoreD(void* R, bool descend); // index.pas r53 ASM
	void StoreN(void* N, WORD len, bool descend); // index.pas r62 ASM
	void StoreF(void* F, WORD len, bool descend); // index.pas r68 ASM
	void StoreA(void* A, WORD len, bool compLex, bool descend); // index.pas r76 ASM
	void negate_esdi(void* data, size_t len);
};
