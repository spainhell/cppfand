#pragma once

#include <string>

#include "FileD.h"
#include "switches.h"
#include "../Common/typeDef.h"

void ScrGraphMode(bool Redraw, WORD OldScrSeg);
WORD ScrTextMode(bool Redraw, bool Switch);

void InitRunFand(); // !!! spusteni - vstupni procedura
void DeleteFandFiles(); // remove Fand work files
void OpenFileDialog();

bool SetTopDir(std::string& p, std::string& n);
void CompileHelpCatDcl();