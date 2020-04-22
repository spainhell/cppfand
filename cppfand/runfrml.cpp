#include "runfrml.h"

pstring runfrml::LeadChar(char C, pstring S)
{
	// TODO: øešit elegantnìji ...
	while (S.length() > 0 && (S[1] == C))
	{
		S = S.substr(1);
	}
	return S;
}

/// funkce má asi oøíznout všechny C na konci øetìzce?
pstring runfrml::TrailChar(char C, pstring S)
{
	while ((S.length() > 0) && (S[S.length()] == C))
	{
		S[S.length()] = '\0';
		S[0] -= 1;
	}
	return S;
}

