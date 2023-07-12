#include "Instr.h"

Instr::Instr(PInstrCode kind)
{
	this->Kind = kind;
}

Instr_menu::Instr_menu(PInstrCode Kind): Instr(Kind)
{
}

Instr_menu::~Instr_menu()
{
	for (const auto& choice : Choices)	{
		delete choice;
	}
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

Instr_proc::Instr_proc(size_t TArg_Count) : Instr(PInstrCode::_proc)
{
}

Instr_lproc::Instr_lproc() : Instr(PInstrCode::_lproc)
{
}

Instr_call::Instr_call() : Instr(PInstrCode::_call)
{
}

Instr_exec::Instr_exec() : Instr(PInstrCode::_exec)
{
}

Instr_copyfile::Instr_copyfile() : Instr(PInstrCode::_copyfile)
{
}

Instr_writeln::Instr_writeln() : Instr(PInstrCode::_writeln)
{
}

Instr_gotoxy::Instr_gotoxy() : Instr(PInstrCode::_gotoxy)
{
}

Instr_assign::Instr_assign(PInstrCode Kind) : Instr(Kind)
{
}

Instr_help::Instr_help() : Instr(PInstrCode::_help)
{
}

Instr_recs::Instr_recs(PInstrCode Kind) : Instr(Kind)
{
}

Instr_turncat::Instr_turncat() : Instr(PInstrCode::_turncat)
{
}

Instr_sort::Instr_sort() : Instr(PInstrCode::_sort)
{
	//SK = new KeyFldD();
}

Instr_sort::~Instr_sort()
{
	delete SK;
}

Instr_edit::Instr_edit() : Instr(PInstrCode::_edit)
{
}

Instr_report::Instr_report() : Instr(PInstrCode::_report)
{
}

Instr_edittxt::Instr_edittxt(PInstrCode Kind) : Instr(Kind)
{
}

Instr_puttxt::Instr_puttxt() : Instr(PInstrCode::_puttxt)
{
}

Instr_releasedrive::Instr_releasedrive() : Instr(PInstrCode::_releasedrive)
{
}

Instr_mount::Instr_mount() : Instr(PInstrCode::_mount)
{
}

Instr_indexfile::Instr_indexfile() : Instr(PInstrCode::_indexfile)
{
}

Instr_getindex::Instr_getindex() : Instr(PInstrCode::_getindex)
{
}

Instr_window::Instr_window() : Instr(PInstrCode::_window)
{
}

Instr_clrww::Instr_clrww() : Instr(PInstrCode::_clrww)
{
}

Instr_forall::Instr_forall() : Instr(PInstrCode::_forall)
{
}

Instr_withshared::Instr_withshared(PInstrCode Kind) : Instr(Kind)
{
}

Instr_graph::Instr_graph() : Instr(PInstrCode::_graph)
{
}

Instr_putpixel::Instr_putpixel(PInstrCode Kind) : Instr(Kind)
{
}

Instr_backup::Instr_backup(PInstrCode Kind) : Instr(Kind)
{
}

Instr_closefds::Instr_closefds() : Instr(PInstrCode::_closefds)
{
}

Instr_setedittxt::Instr_setedittxt() : Instr(PInstrCode::_setedittxt)
{
}

Instr_setmouse::Instr_setmouse() : Instr(PInstrCode::_setmouse)
{
}

Instr_checkfile::Instr_checkfile() : Instr(PInstrCode::_checkfile)
{
}

Instr_login::Instr_login() : Instr(PInstrCode::_login)
{
}

Instr_sqlrdwrtxt::Instr_sqlrdwrtxt() : Instr(PInstrCode::_sqlrdwrtxt)
{
}

Instr_portout::Instr_portout() : Instr(PInstrCode::_portout)
{
}
