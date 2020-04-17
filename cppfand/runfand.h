#pragma once
#include <string>

#include "constants.h"
#include "switches.h"
#include "wwmenu.h"

using namespace std;

typedef TMenuBoxS* PMenuBoxS;

/*#include dos,graph,drivers, base, obase, obaseww, access, oaccess, rdrun,
{ $ifdef FandSQL } channel, { $endif }
{$ifdef FandDML} dml, { $endif }
//wwmenu, wwmix, compile, rdfildcl, editor, runedi, runproj, runfrml;*/

class runfand
{
public:
	void ScrGraphMode(bool Redraw, WORD OldScrSeg);
	WORD ScrTextMode(bool Redraw, bool Switch);
	void InitRunFand(); // !!! spuštìní - vstupní procedura

private:
	void* Ovr();
	bool IsAT();
	void OpenXMS();
	void OpenCache();
	void DetectVideoCard();
	void InitDrivers();
	void InitAccess();
	void RdCFG();
	void RdPrinter();
	void RdWDaysTab();
	void CompileHelpCatDcl();

	bool SetTopDir(string path /*PathStr*/, string name /*NameStr*/);
	void RunRdb(string path);
	void SelectRunRdb(bool OnFace);
	void CallInstallRdb();
	void CallEditTxt();
	void SelectEditTxt(string ExtStr, bool OnFace);
	
};

