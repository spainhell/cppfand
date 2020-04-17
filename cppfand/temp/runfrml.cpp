#include "runfrml.h"

/// funkce má asi oøíznout všechny C na konci øetìzce?
string runfrml::TrailChar(char C, string S)
{
	// ze vstupního stringu udìláme pole znakù
	char* tmpchar = new char[S.length()];
	S.copy(tmpchar, S.length());
	tmpchar[S.length()] = '\0';
	
	/*while (Length(S)>0) and (S[Length(S)]=C) do S[0]:=chr(length(S)-1); TrailChar:=S;*/
	while (strlen(tmpchar) > 0 && strrchr(tmpchar, C))
	{
		*strrchr(tmpchar, C) = '\0';
	}
	string result(tmpchar);
	delete[] tmpchar;
	tmpchar = nullptr;
	return result;
}
