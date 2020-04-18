#include "keybd.h"

bool KbdTimer(WORD Delta, BYTE Kind)
{
	/*
	function KbdTimer(Delta:word;Kind:byte):boolean;
	var EndTime:longint;
	label 1;
	begin
	  EndTime:=Timer+Delta; KbdTimer:=false;
	1:case Kind of          { 0 - wait, 1 - wait or ESC, 2 - wait or any key }
	    1: if KeyPressed and (ReadKey=_ESC_) then exit;
	    2: if KbdPressed then begin ReadKbd; exit end end;
	  if Timer<EndTime then goto 1;
	  KbdTimer:=true;
	end;
	 */

	return true;
}
