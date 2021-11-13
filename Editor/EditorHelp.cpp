#include "EditorHelp.h"

#include <stdexcept>


#include "../cppfand/base.h"
#include "../cppfand/FileD.h"
#include "../cppfand/GlobalVariables.h"
#include "../cppfand/obaseww.h"
#include "../cppfand/wwmenu.h"

// ***********HELP**********  // r351
const BYTE maxStk = 15; WORD iStk = 0;
struct structStk { RdbD* Rdb; FileD* FD; WORD iR, iT; } Stk[maxStk];
void ViewHelpText(LongStr* S, WORD& TxtPos);

void Help(RdbD* R, pstring Name, bool InCWw)
{
	void* p = nullptr; ExitRecord er; FileD* fd = nullptr;
	WORD i, l, l2; WORD iRec, oldIRec;
	LongStr* s = nullptr; LongStr* s2 = nullptr;
	WORD* os = (WORD*)s; WORD* os2 = (WORD*)s2;
	integer delta; bool frst, byName, backw;
	FileD* cf2;
	WORD KbdChar = Event.Pressed.KeyCombination();

	if (R == nullptr) {
		if (iStk == 0) return;
		R = Stk[iStk].Rdb;
		backw = true;
	}
	else { if (Name == "") return; backw = false; }
	if (R == (RdbD*)HelpFD) {
		fd = HelpFD;
		if (HelpFD->Handle == nullptr) {
			WrLLF10Msg(57);
			return;
		}
	}
	else {
		fd = R->HelpFD;
		if (fd == nullptr) return;
	}
	MarkStore(p);
	FileD* cf = CFile;
	longint w = 0, w2 = 0;
	//NewExit(Ovr(), er);
	//goto label4;
	try {
		if (InCWw) {
			WORD c1 = WindMin.X; WORD c2 = WindMax.X;
			WORD r1 = WindMin.Y; WORD r2 = WindMax.Y;
			if ((c1 == 1) && (c2 == TxtCols) && (r1 == 2) && (r2 == TxtRows)) {
				r1 = 1;
			}
			w = PushW(1, TxtRows, TxtCols, TxtRows);
			w2 = PushW1(c1, r1, c2, r2, true, true);
		}
		else w = PushW1(1, 1, TxtCols, TxtRows, true, true);
		i = 1; frst = true; delta = 0;
		if (backw) {
			byName = false;
			goto label3;
		}
	label1:
		byName = true;
	label2:
		s = GetHlpText(R, Name, byName, iRec);
		cf2 = CFile;
		if (s == nullptr)
			if (frst && (R == (RdbD*)(&HelpFD)) && (KbdChar == _CtrlF1_)) {
				KbdChar = 0;
				Name = "Ctrl-F1 error";
				goto label1;
			}
			else {
				SetMsgPar(Name, fd->Name);
				WrLLF10Msg(146);
			}
		else {
			frst = false; byName = false;
			s2 = s;
			if ((s->LL > 0) && (s->A[1] == '{')) {
				//view after 1. line
				l = FindCtrlM(s, 1, 1);
				l = SkipCtrlMJ(s, l) - 1;
				os2 += l; s2->LL = s->LL - l;
			}
			if (s2->LL == 0) {
				if (delta == 0) { /*goto label4*/ throw std::invalid_argument("Editor::Help - delta is equal 0"); }
				else {
					if (iRec != oldIRec) {
						oldIRec = iRec;
						iRec += delta;
						goto label2;
					}
				}
			}
			ViewHelpText(s2, i);
			if (iStk < maxStk) {
				iStk++;
			}
			else {
				Move(&Stk[2], Stk, sizeof(Stk) - 4);
			}
			/* !!! with Stk[iStk] do!!! */
			Stk[iStk].Rdb = R; Stk[iStk].FD = cf2;
			Stk[iStk].iR = iRec; Stk[iStk].iT = i;
			oldIRec = iRec; i = 1; delta = 0;
			ReleaseStore(s);
			CFile = cf2;

			switch (KbdChar) {
			case __ESC: break;
			case __F10: {
				iStk--;
			label3:
				if (iStk > 0) {
					/* !!! with Stk[iStk] do!!! */
					R = Stk[iStk].Rdb; CFile = Stk[iStk].FD;
					iRec = Stk[iStk].iR; i = Stk[iStk].iT;
					iStk--;
					goto label2;
				}
			}
			case __CTRL_HOME: {
				iRec--;
				delta = -1;
				goto label2;
				break;
			}
			case __CTRL_END: {
				iRec++;
				delta = 1;
				goto label2;
				break;
			}
			default: {
				if (KbdChar == __F1) {
					Name = "root";
				}
				else {
					Name = LexWord;
				}
				goto label1;
			}
			}
		}
	}
	catch (std::exception& e) {
		//label4:
		RestoreExit(er);
	}
	if (w2 != 0) { PopW(w2); }
	if (w != 0) { PopW(w); }
	ReleaseStore(p);
	CFile = cf;
}

void ClearHelpStkForCRdb()
{
	WORD i = 1;
	while (i <= iStk) {
		if (Stk[i].Rdb == CRdb) { Move(&Stk[i + 1], &Stk[i], (iStk - i) * 12); iStk--; }
		else { i++; }
	}
}
