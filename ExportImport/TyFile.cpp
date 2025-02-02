#include "TyFile.h"
#include "../Core/constants.h"

TyFile::TyFile(): TcFile(false)
{
}

void TyFile::MountVol(bool is_first)
{
	if (is_first) {
		Floppy = (!Vol.empty()) && (Vol[0] != '#') && (drive_letter - '@' < FloppyDrives);
	}

	// TODO: rest is for a floppy -> not supported

	/*
	if isFirst then begin
		Drive:=ord(upcase(DrvNm[1]))-ord('@');
		Floppy:=(Vol<>'') and (Vol[1]<>'#') and (Drive<FloppyDrives)
	end;
	if not Floppy then exit;
	if not isFirst then inc(Vol[length(Vol)]);
	CVol:=Vol;
	if not SEquUpcase(MountedVol[Drive],Vol) then begin
	    ReleaseDrive(Drive);
	1:
		F10SpecKey:=_ESC_;
		Set2MsgPar(drvNm,Vol);
		WrLLF10Msg(808);
	    if KbdChar=_ESC_ then if PromptYN(21) then GoExit else goto 1
	end;
	2:
	FindFirst(drvNm+':\*.*',VolumeID,s);
	case DosError of
		31,158,162{hardware failure}:
			if IsBackup and PromptYN(655) and FormatOnDrive(DrvNm[1]) then begin
				MountedVol[Drive]:=Vol; exit
			end
			else goto 1;
	    152{drive not ready}: goto 1;
	    18{label missing}:
			if IsBackup then
				if PromptYN(807) then goto 3
				else goto 1
			else begin WrLLF10Msg(809); goto 1 end;
	    0: ;
	    else WrLLF10Msg(810);
	goto 1
	end;

	i:=pos('.',s.Name);
	if i<>0 then delete(s.Name,i,1);
	if not SEquUpcase(s.Name,Vol) then begin
	    SetMsgPar(s.Name);
	    if IsBackup then if spec.OverwrLabeledDisk and PromptYN(816) then goto 3 else goto 1;
	    WrLLF10Msg(817); goto 1
	end;
	3:
	MountedVol[Drive]:=Vol;
	  if not IsBackup then exit;
	  SetMsgPar(drvNm);
	  ResetDisks; bt.Init(Drive-1);
	  i:=bt.ReadSect(0,1,bt.Boot); if i<>0 then RunError(656);
	  fat:=GetZStore(bt.SecSize*bt.SecsPerFat);
	  bt.ReadSect(bt.ReservedSecs,bt.SecsPerFat,fat^);
	  FatPut(fat,0,$ff00+bt.MediaCode); FatPut(fat,1,$ffff);
	  for i:=2 to ((2*longint(bt.SecsPerFat)*bt.SecSize)div 3)-1 do begin
	    j:=FatGet(fat,i); if j<>fatBadCluster then FatPut(fat,i,0) end;
	  for i:=0 to bt.FatCount-1 do begin
	    err:=bt.WriteSect(bt.ReservedSecs+i*bt.SecsPerFat,bt.SecsPerFat,fat^);
	    if err<>0 then begin
	      if err=3 then WrLLF10Msg(850) else WrLLF10Msg(860); goto 1 end end;
	  p:=PDirEntryArr(GetZStore(bt.SecsPerRoot*bt.SecSize));
	  FillVolDirEntry(p^,Vol); bt.WriteSect(bt.RootSec,bt.SecsPerRoot,p^);
	  ResetDisks; ReleaseStore(fat);
	*/

}
