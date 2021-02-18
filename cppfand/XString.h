#pragma once
#include "constants.h"
#include "pstring.h"

class KeyFldD;
class FrmlListEl;

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
	bool PackFrml(FrmlListEl* FL, KeyFldD* KF);
#ifdef FandSQL
	void GetF(WORD Off, WORD Len, bool Descend, void* Buf);
	void GetD(WORD Off, bool Descend, void* R);
	void GetN(WORD Off, WORD Len, bool Descend, void* Buf);
	WORD GetA(WORD Off, WORD Len, bool CompLex, bool Descend, void* Buf);
#endif
private:
	void StoreD(void* R, bool Descend); // index.pas r53 ASM
	void StoreN(void* N, WORD Len, bool Descend); // index.pas r62 ASM
	void StoreF(void* F, WORD Len, bool Descend); // index.pas r68 ASM
	void StoreA(void* A, WORD Len, bool CompLex, bool Descend); // index.pas r76 ASM
};
