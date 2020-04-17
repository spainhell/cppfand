#include "base.h"
#include "access.h"
#include "handle.h"
#include "legacy.h"


void RdMsg(integer N)
{
	WORD j, h, o;
	string s;
	for (int i = 1; i < MsgIdxN; i++) {
		auto Nr = MsgIdx[i].Nr;
		auto Count = MsgIdx[i].Count;
		auto Ofs = MsgIdx[i].Ofs;
		if (N >= Nr && N < Nr + Count)
		{
			j = N - Nr + 1;
			o = Ofs;
			goto label1;
		}
	}
	o = 0; j = 1;
	MsgPar[1] = to_string(N);

label1:
	h = ResFile.Handle;
	SeekH(h, FrstMsgPos + o);

	for (int i = 1; i < j; i++)
	{
		ReadH(h, 1, reinterpret_cast<void*>(s[0])); // tady se má zøejmì jen vyèíst délka
		ReadH(h, s.length(), reinterpret_cast<void*>(s[1]));
	}
	Drivers::ConvKamenToCurr(s[1], s.length());
	MsgLine = "";
	j = 1;
	// TODO: k èemu je toto? s[length(s) + 1] = 0x00;
	for (int i = 1; i < s.length(); i++) {
		if (s[i] == '$' && s[i + 1] != '$')
		{
			MsgLine += MsgPar[j];
			j++;
		}
		else
		{
			MsgLine += s[i];
			if (s[i] == '$') i++;
		}
	}

}

void OpenWorkH()
{
	CPath = FandWorkName;
	CVol = "";
	WorkHandle = OpenH(_isoldnewfile, Exclusive);
	if (HandleError != 0)
	{
		printf("can't open %s", FandWorkName.c_str());
		wait(); Halt(0);
	}
}
