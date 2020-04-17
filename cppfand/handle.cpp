#pragma once

#include "handle.h"
#include "legacy.h"
#include "memory.h"


WORD ReadH(WORD handle, WORD bytes, void* buffer)
{
	if (handle == 0xff) RunError(706);
	// ASM kód:
	/*
	var r:registers;
	begin with r do begin
		if handle=$ff then RunError(706);
		AH:=$3F; DS:=seg(buffer); DX:=ofs(buffer); CX:=bytes; BX:=handle;
		MsDos(r); SetRes(FLAGS,AX); ReadH:=AX;
	end end;
	 */
	// vrátíme natvrdo $0420 - vlk se nažral a koza zùstala úplnì celá
	WORD* wordptr = static_cast<WORD*>(buffer);
	*wordptr = WORD(0x0420);
	return 0;
}

longint PosH(WORD handle)
{
	return MoveH(0, 1, handle);
}

longint MoveH(longint dist, WORD method, WORD handle)
{
	/*	var r:registers;
		begin with r do begin
			AH:=$42; AL:=method; BX:=handle;
			CX:=memW[seg(dist):ofs(dist)+2]; DX:=memW[seg(dist):ofs(dist)];
			 MsDos(r); SetRes(FLAGS,AX); MoveH:=(longint(DX) shl 16)+AX;
		end end;
	*/
	// tož fèil zme v øiti
	// popis memW: https://www.freepascal.org/docs-html/rtl/system/memw.html
	// 
	return -1;
}

void SeekH(WORD handle, longint pos)
{
	if (handle == 0xff) RunError(705);
	MoveH(pos, 0, handle);
}

void CloseClearH(WORD& h)
{
	if (h == 0xFF) exit(0);
	CloseH(h);
	ClearCacheH(h);
	h = 0xFF;
}

bool IsUpdHandle(WORD H)
{
	return H != 0xFF && UpdHandles.count(H) > 0;
}

void TruncH(WORD handle, longint N)
{
	if (handle == 0xff) exit(0);
	if (FileSizeH(handle) > N) {
		SeekH(handle, N);
		WriteH(handle, 0, *ptr(0, 0));
	}
}

longint FileSizeH(WORD handle)
{
	longint pos;
	pos = PosH(handle);
	longint result = MoveH(0, 2, handle);
	SeekH(handle, pos);
	return result;
}

void WriteH(WORD handle, WORD bytes, void* buffer)
{
	if (handle == 0xff) RunError(706);
	// ASM :-(
	/*AH: = $40; DS: = seg(buffer); DX: = ofs(buffer); CX: = bytes; BX: = handle;
	MsDos(r); SetRes(FLAGS, AX); SetUpdHandle(handle);
	if (HandleError = 0) and (AX<>bytes) then HandleError : = 1;*/
}
