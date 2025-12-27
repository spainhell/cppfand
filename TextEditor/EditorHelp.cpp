#include "EditorHelp.h"

#include <memory>
#include <stdexcept>

#include "HelpViewer.h"
#include "../Core/printtxt.h"
#include "../Drivers/constants.h"
#include "../Core/base.h"
#include "../Core/Compiler.h"
#include "../Common/FileD.h"
#include "../Core/GlobalVariables.h"
#include "../Core/obaseww.h"
#include "../Core/wwmenu.h"

// ***********HELP**********  // r351
const uint8_t maxStk = 15;
WORD iStk = 0;
struct structStk { RdbD* Rdb; FileD* FD; WORD iR, iT; } Stk[maxStk];


void FandHelp(FileD* help_file, const std::string& name, bool InCWw)
{
	std::unique_ptr<RdbD> R = std::make_unique<RdbD>();
	R->help_file = help_file;
	Help(R.get(), name, InCWw);
}

void Help(RdbD* R, std::string name, bool InCWw)
{
	uint8_t* p = nullptr;
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
		if (name.empty()) return;
		backw = false;
	}

	if (R->help_file == HelpFD) {
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
	FileD* cf = nullptr; //CFile;
	int w = 0, w2 = 0;

	try {
		bool byName;
		WORD l;
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
		size_t i = 0;
		bool frst = true;
		int16_t delta = 0;
		if (backw) {
			byName = false;
			if (iStk > 0) {
				R = Stk[iStk].Rdb;
				cf = Stk[iStk].FD;
				iRec = Stk[iStk].iR;
				i = Stk[iStk].iT;
				iStk--;
			}
		}
		else {
			byName = true;
		}

		while (true) {
			s = GetHlpText(R, name, byName, iRec);
			FileD* cf2 = cf;
			if (s.empty()) {
				if (frst && (R->help_file == HelpFD) && (KbdChar == __CTRL_F1)) {
					KbdChar = 0;
					name = "Ctrl-F1 error";
					byName = true;
					continue;
				}
				else {
					SetMsgPar(name, fd->Name);
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
					s2 = s.substr(l, s.length() - l);
				}
				else {
					s2 = s;
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

				while (true) {
					std::unique_ptr<HelpViewer> editor = std::make_unique<HelpViewer>();
					editor->ViewHelp(s2, i);
					if (Event.Pressed.KeyCombination() == __F6) {
						PrintArray((char*)s2.c_str(), s2.length(), true);
						continue;
					}
					break;
				}

				if (iStk < maxStk) {
					iStk++;
				}
				else {
					//Move(&Stk[2], Stk, sizeof(Stk) - 4);
					memcpy(Stk, &Stk[2], sizeof(Stk - 4));
				}

				Stk[iStk].Rdb = R; Stk[iStk].FD = cf2;
				Stk[iStk].iR = iRec; Stk[iStk].iT = i;
				oldIRec = iRec; i = 1; delta = 0;
				cf = cf2;

				KbdChar = Event.Pressed.KeyCombination();
				switch (KbdChar) {
				case __ESC: break;
				case __F10: {
					iStk--;
					if (iStk > 0) {
						R = Stk[iStk].Rdb;
						cf = Stk[iStk].FD;
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
						name = "root";
					}
					else {
						name = gc->LexWord;
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

	ClrEvent();
	Event.Pressed.Char = '\0';

	if (w2 != 0) {
		PopW(w2);
	}

	if (w != 0) {
		PopW(w);
	}

	ReleaseStore(&p);
	//CFile = cf;
}

void ClearHelpStkForCRdb()
{
	WORD i = 1;
	while (i <= iStk) {
		if (Stk[i].Rdb == CRdb) {
			//Move(&Stk[i + 1], &Stk[i], (iStk - i) * 12);
			memcpy(&Stk[i], &Stk[i + 1], (iStk - i) * 12);
			iStk--;
		}
		else {
			i++;
		}
	}
}
