#pragma once
#include "constants.h"
#include "access.h"

struct GraphVD : public Chained
{
	//GraphVD* Chain;
	FrmlElem* XZ, *YZ, *Velikost; /*float*/
	FrmlElem* BarPis, *Text; /*pstring*/
};

struct GraphWD : public Chained
{
	//GraphWD* Chain;
	FrmlPtr XZ, YZ, XK, YK; /*float*/
	FrmlPtr BarPoz, BarPis, Text; /*pstring*/
};

struct GraphRGBD : public Chained
{
	// GraphRGBD* Chain;
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
	FileDPtr FD;
	FrmlPtr GF;
	FieldDPtr X, Y, Z;
	FieldDPtr ZA[10];
	FrmlPtr HZA[10];
	FrmlPtr T, H, HX, HY, HZ, C, D, R, P, CO, Assign, Cond; /*pstring*/
	FrmlPtr S, RS, RN, Max, Min, SP; /*float*/
	bool Interact;
	GraphVD* V;
	GraphWD* W;
	GraphRGBD* RGB;
	KeyInD* KeyIn;
	bool SQLFilter;
	KeyDPtr ViewKey;
	WinG* WW;
};

struct ViewPortType {
	integer x1 = 0, y1 = 0, x2 = 0, y2 = 0;
	bool Clip = false;
};

void GetViewSettings(ViewPortType& vp);
void SetViewPort(WORD c1, WORD r1, WORD c2, WORD r2, bool b);
void RectToPixel(WORD c1, WORD r1, WORD c2, WORD r2, WORD& x1, WORD& y1, WORD& x2, WORD& y2);
void RunAutoGraph(FieldList FL, KeyD* VK, FrmlElem* Bool);
void RunBGraph(GraphD* PD, bool AutoGraph);