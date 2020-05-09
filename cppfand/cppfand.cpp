// cppfand.cpp : Tento soubor obsahuje funkci main. Provádění programu se tam zahajuje a ukončuje.
//
#include "constants.h"

#include <iostream>
#include <vector>
#include <SDKDDKVer.h>


#ifdef FandSQL
#include "channel.h"
#endif

#ifdef FandDML
#include "dml.h"
#endif

#ifdef FandGraph
#include "graph.h"
#include "rungraph.h"
#include "GrGlob.h"
#include "GrInit.h"
#include "GrMenu.h"
#include "GrMenu1.h"
#include "Gr2D.h"
#include "Gr3DQ.h"
#include "Gr3DD.h"
#include "GrPoly.h"
#include "GrTransF.h"
#include "rdprolg.h"
#include "runprolg.h"
#endif

std::vector<std::string> paramstr2;

int main(int argc, char* argv[])
{
	for (int i=0; i< argc; i++)
	{
        paramstr2.push_back(argv[i]);
	}
}

