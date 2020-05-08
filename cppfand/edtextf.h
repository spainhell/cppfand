#pragma once
#include "constants.h"

bool RdNextPart();
void LastLine(WORD from, WORD num, WORD& Ind, WORD& Count);
bool RdPredPart();
void FirstLine(WORD from, WORD num, WORD& Ind, WORD& Count);

void UpdateFile();
void RdPart();
void NullChangePart();
void RdFirstPart();
void OpenTxtFh(char Mode);
