#include "EditorHelp.h"

#include <memory>
#include <stdexcept>

#include "TextEditor.h"
#include "../Drivers/constants.h"
#include "../Core/base.h"
#include "../Core/FileD.h"
#include "../Core/GlobalVariables.h"
#include "../Core/obaseww.h"
#include "../Core/wwmenu.h"

// ***********HELP**********  // r351
const BYTE maxStk = 15;
WORD iStk = 0;
struct structStk { RdbD* Rdb; FileD* FD; WORD iR, iT; } Stk[maxStk];

void Help(RdbD* R, pstring Name, bool InCWw)
{
	void* p = nullptr;
	FileD* fd = nullptr;
	WORD l2;
	bool backw;
	WORD KbdChar = Event.Pressed.KeyCombination();

	if (R == nullptr) {
		if (iStk == 0) return;
		R = Stk[iStk].Rdb;
		backw = true;
	}
	else {
		if (Name == "") return;
		backw = false;
	}

	if (R == (RdbD*)HelpFD) {
		fd = HelpFD;
		if (HelpFD->FF->Handle == nullptr) {
			WrLLF10Msg(57);
			return;
		}
	}
	else {
		fd = R->help_file;
		if (fd == nullptr) return;
	}

	MarkStore(p);
	FileD* cf = CFile;
	int w = 0, w2 = 0;

	try {
		FileD* cf2;
		bool byName;
		bool frst;
		WORD l;
		short delta;
		WORD i;
		WORD oldIRec;
		std::string s2;
		WORD iRec;
		std::string s;
		if (InCWw) {
			WORD c1 = WindMin.X;
			WORD c2 = WindMax.X;
			WORD r1 = WindMin.Y;
			WORD r2 = WindMax.Y;
			if (c1 == 1 && c2 == TxtCols && r1 == 2 && r2 == TxtRows) {
				r1 = 1;
			}
			w = PushW(1, TxtRows, TxtCols, TxtRows);
			w2 = PushW(c1, r1, c2, r2, true, true);
		}
		else {
			w = PushW(1, 1, TxtCols, TxtRows, true, true);
		}
		i = 1;
		frst = true;
		delta = 0;
		if (backw) {
			byName = false;
			if (iStk > 0) {
				R = Stk[iStk].Rdb;
				CFile = Stk[iStk].FD;
				iRec = Stk[iStk].iR;
				i = Stk[iStk].iT;
				iStk--;
			}
		}
		else {
			byName = true;
		}

		while (true) {
			s = GetHlpText(R, Name, byName, iRec);
			cf2 = CFile;
			if (s.empty()) {
				if (frst && (R == (RdbD*)(&HelpFD)) && (KbdChar == __CTRL_F1)) {
					KbdChar = 0;
					Name = "Ctrl-F1 error";
					byName = true;
					continue;
				}
				else {
					SetMsgPar(Name, fd->Name);
					WrLLF10Msg(146);
				}
			}
			else {
				frst = false;
				byName = false;
				if (!s.empty() && s[0] == '{') {
					//view after 1. line
					l = FindCtrlM(s, 0, 1);
					l = SkipCtrlMJ(s, l);
					// char* newA = new char[s->LL - l];
					// memcpy(newA, &s->A[l], s->LL - l);
					s2 = s.substr(l, s.length() - l); // new LongStr(newA, s->LL - l);
				}
				else {
					// char* newA = new char[s->LL];
					// memcpy(newA, s->A, s->LL);
					s2 = s; // new LongStr(s->A, s->LL);
				}
				if (s2.empty()) {
					if (delta == 0) { /*goto label4*/
						throw std::invalid_argument("Editor::Help - delta is equal 0");
					}
					else {
						if (iRec != oldIRec) {
							oldIRec = iRec;
							iRec += delta;
							continue;
						}
					}
				}
				std::unique_ptr<TextEditor> editor = std::make_unique<TextEditor>();
				editor->ViewHelpText(s2, i);
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
				// ReleaseStore(s);
				// ReleaseStore(s2);
				CFile = cf2;

				KbdChar = Event.Pressed.KeyCombination();
				switch (KbdChar) {
				case __ESC: break;
				case __F10: {
					iStk--;
					if (iStk > 0) {
						R = Stk[iStk].Rdb;
						CFile = Stk[iStk].FD;
						iRec = Stk[iStk].iR;
						i = Stk[iStk].iT;
						iStk--;
						continue;
					}
				}
				case __CTRL_HOME: {
					iRec--;
					delta = -1;
					continue;
					break;
				}
				case __CTRL_END: {
					iRec++;
					delta = 1;
					continue;
					break;
				}
				default: {
					if (KbdChar == __F1) {
						Name = "root";
					}
					else {
						Name = LexWord;
					}
					byName = true;
					continue;
					break;
				}
				}
			}
			break;
		}
	}
	catch (std::exception& e) {
		// TODO: log error
	}
	if (w2 != 0) {
		PopW(w2);
	}
	if (w != 0) {
		PopW(w);
	}
	ReleaseStore(&p);
	CFile = cf;
}

void ClearHelpStkForCRdb()
{
	WORD i = 1;
	while (i <= iStk) {
		if (Stk[i].Rdb == CRdb) {
			Move(&Stk[i + 1], &Stk[i], (iStk - i) * 12);
			iStk--;
		}
		else {
			i++;
		}
	}
}
