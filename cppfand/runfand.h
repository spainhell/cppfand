#pragma once
#include "pstring.h"
#include "constants.h"
#include "switches.h"


using namespace std;

//typedef TMenuBoxS* PMenuBoxS;

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

	bool SetTopDir(pstring& path, pstring& name);
	void RunRdb(pstring path);
	void SelectRunRdb(bool OnFace);
	void CallInstallRdb();
	void CallEditTxt();
	void SelectEditTxt(pstring E, bool OnFace);
	
};

