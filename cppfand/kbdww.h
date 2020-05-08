//#pragma once
//#include "base.h"
//#include "constants.h"
//#include "pstring.h"
//
//// *** DISPLAY ***
//WORD LenStyleStr(pstring s);
//WORD LogToAbsLenStyleStr(pstring s, WORD l);
//
//pstring CStyle(10); pstring CColor(11);
//
//bool SetStyleAttr(char c, BYTE& a);
//void WrStyleChar(char c);
//void WrStyleStr(pstring s, WORD Attr);
//void WrLongStyleStr(LongStr* S, WORD Attr);
//
//void RectToPixel(WORD c1, WORD r1, WORD c2, WORD r2, WORD& x1, WORD& y1, WORD& x2, WORD& y2); // existuje v COMMON
//void* PushWParam(WORD C1, WORD  R1, WORD  C2, WORD R2, bool WW); // def. v OBASEWW
//void PopWParam(void* p); // def. v OBASEWW
//void* PushScr(WORD C1, WORD R1, WORD C2, WORD R2); // def. v OBASEWW
//longint PushW1(WORD C1, WORD R1, WORD C2, WORD R2, bool PushPixel, bool WW); // def. v OBASEWW
//longint PushW(WORD C1, WORD R1, WORD C2, WORD R2); // def. v OBASEWW
//void PopScr(void* p); // r120 // def. v OBASEWW
//void PopW2(longint pos, bool draw); // r128 // def. v OBASEWW
//void PopW(longint pos); // def. v OBASEWW
//
//void WriteWFrame(BYTE WFlags, pstring top, pstring bottom); // def. v OBASEWW
//void WrHd(pstring Hd, WORD Row, WORD MaxCols); // def. v OBASEWW
//
//void CenterWw(BYTE& C1, BYTE& R1, BYTE& C2, BYTE& R2, BYTE WFlags); // def. v OBASEWW
//longint PushWFramed(BYTE C1, BYTE R1, BYTE C2, BYTE R2, WORD Attr, pstring top, pstring bottom, BYTE WFlags);
//longint PushWrLLMsg(WORD N, bool WithESC);
//
//void WrLLMsg(WORD N); // r220
//void WrLLMsgTxt(); // r234
//void WrLLF10MsgLine(); // r251
//void WrLLF10Msg(WORD N); // r283
//bool PromptYN(WORD NMsg); // r286
//WORD RunErrNr;
//void RunError(WORD N);
