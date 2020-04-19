#pragma once
#include "access.h"
#include "pstring.h"


class runfrml
{
public:
	FileDPtr TFD02; TFilePtr TF02; longint TF02Pos; // r33
	
	static double Owned(FrmlPtr Bool, FrmlPtr Sum, LinkDPtr LD);
	static integer CompBool(bool B1, bool B2);
	static integer CompReal(double R1, double R2, integer M); // r42
	static LongStrPtr CopyToLongStr(const pstring& SS);
	static pstring LeadChar(char C, pstring S); // r69
	static pstring TrailChar(char C, pstring s); // r73

	static LongStrPtr CopyLine(LongStrPtr S, WORD N, WORD M);
	static bool RunBool(FrmlPtr X);
	static double RunReal(FrmlPtr X);
	static longint RunInt(FrmlPtr X);

	static void AssgnFrml(FieldDPtr F, FrmlPtr X, bool Delete, bool Add);
	static void LVAssignFrml(LocVar* LV, void* OldBP, bool Add, FrmlPtr X);
	static void DecodeFieldRSB(FieldDPtr F, WORD LWw, double R, pstring T, bool B, pstring& Txt);
	static void DecodeField(FieldDPtr F, WORD LWw, pstring& Txt);
	static void RunWFrml(WRectFrml X, BYTE WFlags, WRect& W);
	static WORD RunWordImpl(FrmlPtr Z, WORD Impl);
	static bool FieldInList(FieldDPtr F, FieldListEl FL);
	static KeyDPtr GetFromKey(LinkDPtr LD);
	static FrmlPtr RunEvalFrml(FrmlPtr Z);
	static bool CanCopyT(FieldDPtr F, FrmlPtr Z);
	
	static LongStr* RunLongStr(FrmlPtr X);  // r417 zaèíná od 555
	static pstring RunShortStr(FrmlPtr X); // r629 ASM

private:
	static void ConcatLongStr(LongStrPtr S1, LongStrPtr S2); // r418 ASM
	static void CopyLongStr(LongStrPtr S, WORD From, WORD Number); // r425 ASM
	static void AddToLongStr(LongStrPtr S, void* P, WORD L); // r433
	static void StrMask(double R, pstring& Mask); // r438
	static LongStr* RunS(FrmlPtr Z); // r469
	static LongStr* RunSelectStr(FrmlPtr Z); // r522
	static void LowCase(LongStr* S); //543 ASM
	
	
};

