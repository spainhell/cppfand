#pragma once

#include "switches.h"
#include "../Common/typeDef.h"

void ScrGraphMode(bool Redraw, WORD OldScrSeg);
WORD ScrTextMode(bool Redraw, bool Switch);

void InitRunFand(); // !!! spuštìní - vstupní procedura
void DeleteFandFiles(); // remove Fand work files
void OpenFileDialog();