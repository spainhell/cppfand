#pragma once
#include "constants.h"
#include "switches.h"

void ScrGraphMode(bool Redraw, WORD OldScrSeg);
WORD ScrTextMode(bool Redraw, bool Switch);

void InitRunFand(); // !!! spuštìní - vstupní procedura
void DeleteFandFiles(); // remove Fand work files
void OpenFileDialog();