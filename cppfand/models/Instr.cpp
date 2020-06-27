#include "Instr.h"

Instr_menu::Instr_menu(PInstrCode Kind): Instr(Kind)
{
}

Instr_loops::Instr_loops(PInstrCode Kind): Instr(Kind)
{
}

void Instr_loops::AddInstr(Instr* i)
{
	Instr* prelast = Instr1;
	Instr* last = Instr1;
	while (last != nullptr) { prelast = last; last = last->Chain; }
	prelast->Chain = i;
}

void Instr_loops::AddElseInstr(Instr* i)
{
	Instr* prelast = ElseInstr1;
	Instr* last = ElseInstr1;
	while (last != nullptr) { prelast = last; last = last->Chain; }
	prelast->Chain = i;
}

Instr_merge_display::Instr_merge_display(PInstrCode Kind) : Instr(Kind)
{
}

Instr_proc::Instr_proc(size_t TArg_Count) : Instr(_proc)
{
}

Instr_lproc::Instr_lproc() : Instr(_lproc)
{
}

Instr_call::Instr_call() : Instr(_call)
{
}

Instr_exec::Instr_exec() : Instr(_exec)
{
}

Instr_copyfile::Instr_copyfile() : Instr(_copyfile)
{
}

Instr_writeln::Instr_writeln() : Instr(_writeln)
{
}

Instr_gotoxy::Instr_gotoxy() : Instr(_gotoxy)
{
}

Instr_assign::Instr_assign(PInstrCode Kind) : Instr(Kind)
{
}

Instr_help::Instr_help() : Instr(_help)
{
}

Instr_recs::Instr_recs(PInstrCode Kind) : Instr(Kind)
{
}

Instr_turncat::Instr_turncat() : Instr(_turncat)
{
}

Instr_sort::Instr_sort() : Instr(_sort)
{
	SK = new KeyFldD();
}

Instr_sort::~Instr_sort()
{
	delete SK;
}

Instr_edit::Instr_edit() : Instr(_edit)
{
}

Instr_report::Instr_report() : Instr(_report)
{
}

Instr_edittxt::Instr_edittxt(PInstrCode Kind) : Instr(Kind)
{
}

Instr_puttxt::Instr_puttxt() : Instr(_puttxt)
{
}

Instr_releasedrive::Instr_releasedrive() : Instr(_releasedrive)
{
}

Instr_mount::Instr_mount() : Instr(_mount)
{
}

Instr_indexfile::Instr_indexfile() : Instr(_indexfile)
{
}

Instr_getindex::Instr_getindex() : Instr(_getindex)
{
}

Instr_window::Instr_window() : Instr(_window)
{
}

Instr_clrww::Instr_clrww() : Instr(_clrww)
{
}

Instr_forall::Instr_forall() : Instr(_forall)
{
}

Instr_withshared::Instr_withshared(PInstrCode Kind) : Instr(Kind)
{
}

Instr_graph::Instr_graph() : Instr(_graph)
{
}

Instr_putpixel::Instr_putpixel(PInstrCode Kind) : Instr(Kind)
{
}

Instr_backup::Instr_backup(PInstrCode Kind) : Instr(Kind)
{
}

Instr_closefds::Instr_closefds() : Instr(_closefds)
{
}

Instr_setedittxt::Instr_setedittxt() : Instr(_setedittxt)
{
}

Instr_setmouse::Instr_setmouse() : Instr(_setmouse)
{
}

Instr_checkfile::Instr_checkfile() : Instr(_checkfile)
{
}

Instr_login::Instr_login() : Instr(_login)
{
}

Instr_sqlrdwrtxt::Instr_sqlrdwrtxt() : Instr(_sqlrdwrtxt)
{
}

Instr_portout::Instr_portout() : Instr(_portout)
{
}
