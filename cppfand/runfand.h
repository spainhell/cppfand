#pragma once
#include "pstring.h"
#include "constants.h"
#include "switches.h"

void ScrGraphMode(bool Redraw, WORD OldScrSeg);
WORD ScrTextMode(bool Redraw, bool Switch);

void InitRunFand(); // !!! spuštìní - vstupní procedura
void OpenFileDialog();