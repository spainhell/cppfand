#include "shared.h"


#include "rdmerg.h"
#include "rdrprt.h"
#include "../cppfand/compile.h"
#include "../cppfand/GlobalVariables.h"


void TestNotSum()
{
	if (FrmlSumEl != nullptr) OldError(41);
}

void Err(char source, bool wasIiPrefix)
{
	TestNotSum();
	switch (source) {
	case 'r': SetIi_Report(wasIiPrefix); break;
	case 'm': SetIi_Merge(wasIiPrefix); break;
	}
	if (IDA[Ii]->ErrTxtFrml == nullptr)
	{
		IDA[Ii]->ErrTxtFrml = new FrmlElem4(_const, 0); // GetOp(_const, 256);
	}
}

void SetIi_Merge(bool wasIiPrefix)
{
	if (!wasIiPrefix) {
		if (!Join && (WhatToRd == 'i')) Ii = Oi;
		else Ii = 1;
	}
}

void SetIi_Report(bool wasIiPrefix)
{
	if (!wasIiPrefix) {
		if (WhatToRd == 'i') Ii = Oi;
		else Ii = 1;
	}
}