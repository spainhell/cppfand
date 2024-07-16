#pragma once
#include <string>

class FrmlElem;

struct RFldD
{
	char FrmlTyp = '\0';
	char Typ = '\0'; /*rdb,F,D,T*/
	bool BlankOrWrap = false; /*long date "DD.MM.YYYY"*/
	FrmlElem* Frml = nullptr;
	std::string Name; /*curr. length*/
};
