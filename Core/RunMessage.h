#pragma once
#include <cstdint>

struct RunMsgD
{
	RunMsgD* Last = nullptr;
	int MsgNN = 0;
	int MsgStep = 0;
	int MsgKum = 0;
	int W = 0;
};

void RunMsgOn(int8_t C, int32_t N);
void RunMsgOff();
void RunMsgN(int32_t N);
void RunMsgClear();

