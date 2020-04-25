#include "obaseww.h"


#include "base.h"
#include "drivers.h"
#include "kbdww.h"
#include "oaccess.h"

void WrHd(pstring s, string Hd, WORD Row, WORD MaxCols)
{
	if (Hd == "") exit(0);
	s = " " + Hd + " ";
	if (s.length() > MaxCols) { s = s.substr(0, MaxCols); }
	GotoXY((MaxCols - s.length()) / 2 + 2, Row);
	printf(s.c_str());
}

void CFileMsg(WORD n, char Typ)
{
	string s;
	SetCPathVol();
	if (Typ == 'T') CExtToT();
	else if (Typ == 'X') CExtToX();
	SetMsgPar(CPath); WrLLF10Msg(n);
}

void CFileError(WORD N)
{
	CFileMsg(N, '0');
	CloseGoExit();
}

void WriteWFrame(BYTE WFlags, string top, string bottom)
{
	pstring s;
	WORD cols, rows, n;

	if ((WFlags && WHasFrame) == 0) exit(0);
	n = 0;
	if ((WFlags && WDoubleFrame) != 0) n = 9;
	cols = WindMax.X - WindMin.X + 1;
	rows = WindMax.Y - WindMin.Y + 1;
	ScrWrFrameLn(WindMin.X, WindMin.Y, n, cols, TextAttr);
	for (int i = 1; i < rows - 2; i++) {
		if ((WFlags && WNoClrScr) == 0)
			ScrWrFrameLn(WindMin.X, WindMin.Y + i, n + 6, cols, TextAttr);
		else {
			ScrWrChar(WindMin.X, WindMin.Y + i, FrameChars[n + 6], TextAttr);
			ScrWrChar(WindMin.X + cols - 1, WindMin.Y + i, FrameChars[n + 8], TextAttr);
		}
	}
	ScrWrFrameLn(WindMin.X, WindMax.Y, n + 3, cols, TextAttr);
	WrHd(s, top, 1, cols - 2);
	WrHd(s, bottom, rows, cols - 2);

}
