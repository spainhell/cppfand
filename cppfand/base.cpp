#include "base.h"
#include "drivers.h"
#include "handle.h"
#include "kbdww.h"
#include "keybd.h"
#include "legacy.h"
#include "memory.h"

void SetMsgPar(pstring s)
{
	MsgPar[0] = s;
}

void Set2MsgPar(pstring s1, pstring s2)
{
	MsgPar[0] = s1; MsgPar[1] = s2;
}

void Set3MsgPar(pstring s1, pstring s2, pstring s3)
{
	Set2MsgPar(s1, s2);
	MsgPar[2] = s3;
}

void Set4MsgPar(pstring s1, pstring s2, pstring s3, pstring s4)
{
	Set3MsgPar(s1, s2, s3);
	MsgPar[3] = s4;
}

void RdMsg(integer N)
{
	WORD j, o;
	FILE* h;
	pstring s;
	for (int i = 1; i < MsgIdxN; i++) {
		auto Nr = MsgIdx[i].Nr;
		auto Count = MsgIdx[i].Count;
		auto Ofs = MsgIdx[i].Ofs;
		if (N >= Nr && N < Nr + Count)
		{
			j = N - Nr + 1;
			o = Ofs;
			goto label1;
		}
	}
	o = 0; j = 1;
	MsgPar[1] = std::to_string(N);

label1:
	h = ResFile.Handle;
	SeekH(h, FrstMsgPos + o);

	for (int i = 1; i < j; i++)
	{
		ReadH(h, 1, reinterpret_cast<void*>(s[0])); // tady se má zøejmì jen vyèíst délka
		ReadH(h, s.length(), reinterpret_cast<void*>(s[1]));
	}
	ConvKamenToCurr((char*)s.c_str(), s.length());
	MsgLine = "";
	j = 1;
	// TODO: k èemu je toto? s[length(s) + 1] = 0x00;
	for (int i = 1; i < s.length(); i++) {
		if (s[i] == '$' && s[i + 1] != '$')
		{
			MsgLine += MsgPar[j];
			j++;
		}
		else
		{
			MsgLine += s[i];
			if (s[i] == '$') i++;
		}
	}

}

WORD TResFile::Get(WORD Kod, void* P)
{
	WORD l;
	l = A[Kod].Size;
	GetMem(P, l);
	SeekH(Handle, A[Kod].Pos);
	ReadH(Handle, l, P);
	return l;
}

LongStrPtr TResFile::GetStr(WORD Kod)
{
	LongStrPtr s;
	/* !!! with A[Kod] do!!! */
	s = (LongStrPtr)GetStore(A[Kod].Size + 2); s->LL = A[Kod].Size;
	SeekH(Handle, A[Kod].Pos); ReadH(Handle, A[Kod].Size, s->A);
	return s;
}

WORD StackOvr()
{
	/*
	 procedure StackOvr(NewBP:word);
	type SF=record BP:word; case byte of 0:(Ret:pointer); 1:(RetOfs:word) end;
	var p,q:^SF;   pofs:word absolute p; qofs:word absolute q;
	r,t:^word; rofs:word absolute r; tofs:word absolute t;
	label 1;
	begin
	asm mov p.word,bp; mov p[2].word,ss end; pofs:=p^.BP;
	while pofs<NewBP do begin r:=p^.ret; pofs:=p^.BP;
	if (rofs=0) and (r^=$3fcd) then begin
	q:=ptr(SSeg,NewBP); inc(rofs,2);
	while qofs<BPBound do begin t:=q^.ret;
	if (seg(t^)=seg(r^)) and (tofs<>0) then begin
	   r^:=tofs; q^.retofs:=0; goto 1 end;
	qofs:=q^.BP end end;
	1:end;
	end;
	 */
	return 0;
}

void NoOvr()
{
	/*
		procedure NoOvr; far; assembler;
		asm   pop ax; pop ax; pop ax{bp}; push ax; push ax; call StackOvr;
			  pop bp; pop ds; pop ax; pop dx; pop sp; push dx; push ax;
		end;
	 */
}

pstring PrTab(WORD N)
{
	/*
	function PrTab(N:word):string;
		var p:pointer;
		begin p:=printer[prCurr].Strg;
		asm  push ds; cld; lds si,p; les di,@result; mov cx,N; inc cx; xor ax,ax;
		@1:  add si,ax; lodsb; loop @1;
			 stosb; mov cx,ax; rep movsb; pop ds end;
		end;
		procedure SetCurrPrinter(NewPr:integer);
		begin
		  if NewPr>=prMax then exit;
		  if prCurr>=0 then with printer[prCurr] do if TmOut<>0 then
			PrTimeOut[Lpti]:=OldPrTimeOut[Lpti];
		  prCurr:=NewPr;
		  if prCurr>=0 then with printer[prCurr] do if TmOut<>0 then begin
			PrTimeOut[Lpti]:=TmOut end;
		end;
	 */
	return pstring();
}

void SetCurrPrinter(integer NewPr)
{
	if (NewPr >= prMax) return;
	if (prCurr >= 0) /* !!! with printer[prCurr] do!!! */
		if (printer[prCurr].TmOut != 0)	PrTimeOut[printer[prCurr].Lpti] = OldPrTimeOut[printer[prCurr].Lpti];
	prCurr = NewPr;
	if (prCurr >= 0) /* !!! with printer[prCurr] do!!! */
		if (printer[prCurr].TmOut != 0) PrTimeOut[printer[prCurr].Lpti] = printer[prCurr].TmOut;
}

void MyExit()
{
	// { asm mov ax, SEG @Data; mov ds, ax end; }
	ExitProc = ExitSave;
	if (!WasInitPgm) { UnExtendHandles(); goto label1; }

	if (ErrorAddr != nullptr)
		switch (ExitCode)
		{
		case 202: // {stack overflow}
		{
			// asm mov sp, ExitBuf.rSP
			WrLLF10Msg(625);
			break;
		}
		case 209: //{overlay read error}
			WrLLF10Msg(648);
			break;
		default: WrTurboErr(); break;
		}
#ifdef FandSQL
	SQLDisconnect();
#endif

	UnExtendHandles();
	DeleteFile(FandWorkName);
	DeleteFile(FandWorkXName);
	DeleteFile(FandWorkTName);
	// TODO? CloseXMS();
label1: if (WasInitDrivers) {
	// TODO? DoneMouseEvents();
	Drivers::CrsIntrDone();
	Drivers::BreakIntrDone();
	if (IsGraphMode) {
		CloseGraph();
		IsGraphMode = false;
		// TODO? ScrSeg = video.Address;
		/*asm  push bp; mov ah,0fH; int 10H; cmp al,StartMode; je @1;
			 mov ah,0; mov al,StartMode; int 10H;
		@1:  pop bp end; */
		Drivers::Window(1, 1, TxtCols, TxtRows);
		TextAttr = StartAttr;
		Drivers::ClrScr();
		Drivers::CrsNorm();
		ChDir(OldDir);
		SetCurrPrinter(-1);
	}
	if (ExitCode == 202) Halt(202);
}
}

void WrTurboErr()
{
	pstring s = pstring(9);
	str(ExitCode, s);
	SetMsgPar(s);
	WrLLF10Msg(626);
	ErrorAddr = nullptr;
	ExitCode = 0;
}

void OpenResFile()
{
	CPath = FandResName; CVol = "";
	ResFile.Handle = OpenH(_isoldfile, RdOnly);
	if (HandleError != 0)
	{
		printf("can't open %s", FandResName.c_str());
		wait();
		Halt(0);
	}
}

void InitOverlays()
{
	pstring name; pstring ext; integer sz, err; longint l; pstring s;
	const BYTE OvrlSz = 124;

	GetDir(0, OldDir); FSplit(FExpand(ParamStr(0)), FandDir, name, ext);
	FandOvrName = MyFExpand(name + ".OVR", "FANDOVR");
	CPath = FandResName; CVol = ""; ResFile.Handle = OpenH(_isoldfile, RdOnly);
	if (OvrResult != 0) {          /*reshandle-1*/
		FandOvrName = ParamStr(0);
		OvrInit(FandOvrName);
		if (OvrResult != 0) {
			printf("can't open FAND.OVR"); wait(); Halt(-1);
		}
	}
	OvrInitEMS();
	s = GetEnv("FANDOVRB");
	while ((s.length() > 0) && (s[s.length()] == ' ')) s[0] = s.length() - 1;
	val(s, sz, err);
	if ((err != 0) || (sz < 80) || (sz > OvrlSz + 10)) sz = OvrlSz; l = longint(sz) * 1024;
	OvrSetBuf(l);
	OvrSetRetry(l / 2);
	//TODO: FreeList = nullptr;
}

void OpenWorkH()
{
	integer UserLicNr;
	double userToday;
	// TODO:
	// CurPSP = ptr(PrefixSeg, 0);
	// MyHeapEnd = HeapEnd;
	ExtendHandles();
	prCurr = -1;
	InitOverlays();
	ExitSave = ExitProc;
	ExitProc = MyExit;
	MyBP = nullptr;
	UserLicNr = WORD(UserLicNrShow) & 0x7FFF;
	FandResName = MyFExpand("Fand.Res", "FANDRES");
	OpenResFile();
}

void OpenOvrFile()
{
	FILE* h;
	CPath = FandOvrName;
	CVol = "";
	h = OpenH(_isoldfile, RdOnly);
		if (h != OvrHandle)
		{
			printf("can't open FAND.OVR");
			wait();
			Halt(-1);
		}
}
