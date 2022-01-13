#pragma once
#include "constants.h"
#include "access.h"
#include "models/FrmlElem.h"

class FieldDescr;

struct GraphVD : public Chained<GraphVD>
{
	//GraphVD* pChain;
	FrmlElem* XZ, *YZ, *Velikost; /*float*/
	FrmlElem* BarPis, *Text; /*pstring*/
};

struct GraphWD : public Chained<GraphWD>
{
	//GraphWD* pChain;
	FrmlPtr XZ, YZ, XK, YK; /*float*/
	FrmlPtr BarPoz, BarPis, Text; /*pstring*/
};

struct GraphRGBD : public Chained<GraphRGBD>
{
	// GraphRGBD* pChain;
	FrmlPtr Barva; /*pstring*/
	FrmlPtr R, G, B; /*float*/
};

struct WinG
{
	WRectFrml W;
	WRect WR;
	FrmlPtr ColFrame, ColBack, ColFor;  /*pstring*/
	FrmlPtr Top;
	BYTE WFlags;
};

struct GraphD
{
	FileD* FD;
	FrmlElem* GF;
	FieldDescr* X, *Y, *Z;
	FieldDescr* ZA[10];
	FrmlElem* HZA[10];
	FrmlElem* T, *H, *HX, *HY, *HZ, *C, *D, *R, *P, *CO, *Assign, *Cond; /*pstring*/
	FrmlElem* S, *RS, *RN, *Max, *Min, *SP; /*float*/
	bool Interact;
	GraphVD* V;
	GraphWD* W;
	GraphRGBD* RGB;
	KeyInD* KeyIn;
	bool SQLFilter;
	XKey* ViewKey;
	WinG* WW;
};

struct ViewPortType {
	integer x1 = 0, y1 = 0, x2 = 0, y2 = 0;
	bool Clip = false;
};

void GetViewSettings(ViewPortType& vp);
void SetViewPort(WORD c1, WORD r1, WORD c2, WORD r2, bool b);
void RectToPixel(WORD c1, WORD r1, WORD c2, WORD r2, WORD& x1, WORD& y1, WORD& x2, WORD& y2);
void RunAutoGraph(FieldList FL, XKey* VK, FrmlElem* Bool);
void RunBGraph(GraphD* PD, bool AutoGraph);