#pragma once

struct RunMsgD
{
	RunMsgD* Last = nullptr;
	int MsgNN = 0;
	int MsgStep = 0;
	int MsgKum = 0;
	int W = 0;
};

void RunMsgOn(char C, int N);
void RunMsgOff();
void RunMsgN(int N);
void RunMsgClear();

