// cppfand.cpp : Tento soubor obsahuje funkci main. Provádění programu se tam zahajuje a ukončuje.
//

#include <iostream>
#include <vector>


//#include "constants.h"
//#include "switches.h"
//#include "drivers.h"

//#include "base.h"
//#include "obase.h"
//#include "obaseww"
//#include "access.h"
//#include "oaccess.h"
//#include "runfrml.h"
//#include "olongstr"
//#include "rdrun.h"
//#include "wwmenu.h"
//#include "wwmix.h"
//#include "compile.h"
//#include "rdfildcl.h"
//#include "rdmerg.h"
//#include "rdrprt.h"
//#include "rdproc.h"
//#include "rdedit.h"
//#include "sort.h"
//#include "runmerg.h"
//#include "runrprt.h"
//#include "printtxt.h"
//#include "genrprt.h"
//#include "editor.h"
//#include "expimp.h"
//#include "runproc"
//#include "runedi.h"
//#include "runproj.h"
//#include "runfand.h"

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

std::vector<std::string> paramstr;

int main(int argc, char* argv[])
{
	for (int i=0; i< argc; i++)
	{
        paramstr.push_back(argv[i]);
	}
}

