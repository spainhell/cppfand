#include "Instr.h"

Instr::Instr(PInstrCode kind)
{
	this->Kind = kind;
}

Instr::~Instr()
{
}

Instr_menu::Instr_menu(PInstrCode Kind) : Instr(Kind)
{
}

Instr_menu::~Instr_menu()
{
	for (ChoiceD* const& choice : Choices) {
		delete choice;
	}
	for (Instr* const& instr : ESCInstr) {
		delete instr;
	}
}

Instr_loops::Instr_loops(PInstrCode Kind) : Instr(Kind)
{
}

Instr_loops::~Instr_loops()
{
	for (Instr* const& instr : v_instr) {
		delete instr;
	}
	for (Instr* const& instr : v_else_instr) {
		delete instr;
	}
}

Instr_merge_display::Instr_merge_display(PInstrCode Kind) : Instr(Kind)
{
}

Instr_merge_display::~Instr_merge_display()
{
}

Instr_proc::Instr_proc(size_t TArg_Count) : Instr(PInstrCode::_proc)
{
}

Instr_proc::~Instr_proc()
{
}

Instr_lproc::Instr_lproc() : Instr(PInstrCode::_lproc)
{
}

Instr_lproc::~Instr_lproc()
{
}

Instr_call::Instr_call() : Instr(PInstrCode::_call)
{
}

Instr_call::~Instr_call()
{
}

Instr_exec::Instr_exec() : Instr(PInstrCode::_exec)
{
}

Instr_exec::~Instr_exec()
{
}

Instr_copyfile::Instr_copyfile() : Instr(PInstrCode::_copyfile)
{
}

Instr_copyfile::~Instr_copyfile()
{
}

Instr_writeln::Instr_writeln() : Instr(PInstrCode::_writeln)
{
}

Instr_writeln::~Instr_writeln()
{
}

Instr_gotoxy::Instr_gotoxy() : Instr(PInstrCode::_gotoxy)
{
}

Instr_gotoxy::~Instr_gotoxy()
{
}

Instr_assign::Instr_assign(PInstrCode Kind) : Instr(Kind)
{
}

Instr_assign::~Instr_assign()
{
}

Instr_help::Instr_help() : Instr(PInstrCode::_help)
{
}

Instr_help::~Instr_help()
{
}

Instr_recs::Instr_recs(PInstrCode Kind) : Instr(Kind)
{
}

Instr_recs::~Instr_recs()
{
}

Instr_turncat::Instr_turncat() : Instr(PInstrCode::_turncat)
{
}

Instr_turncat::~Instr_turncat()
{
}

Instr_sort::Instr_sort() : Instr(PInstrCode::_sort)
{
}

Instr_sort::~Instr_sort()
{
}

Instr_edit::Instr_edit() : Instr(PInstrCode::_edit)
{
}

Instr_edit::~Instr_edit()
{
}

Instr_report::Instr_report() : Instr(PInstrCode::_report)
{
}

Instr_report::~Instr_report()
{
}

Instr_edittxt::Instr_edittxt(PInstrCode Kind) : Instr(Kind)
{
}

Instr_edittxt::~Instr_edittxt()
{
}

Instr_puttxt::Instr_puttxt() : Instr(PInstrCode::_puttxt)
{
}

Instr_puttxt::~Instr_puttxt()
{
}

Instr_releasedrive::Instr_releasedrive() : Instr(PInstrCode::_releasedrive)
{
}

Instr_releasedrive::~Instr_releasedrive()
{
}

Instr_mount::Instr_mount() : Instr(PInstrCode::_mount)
{
}

Instr_mount::~Instr_mount()
{
}

Instr_indexfile::Instr_indexfile() : Instr(PInstrCode::_indexfile)
{
}

Instr_indexfile::~Instr_indexfile()
{
}

Instr_getindex::Instr_getindex() : Instr(PInstrCode::_getindex)
{
}

Instr_getindex::~Instr_getindex()
{
}

Instr_window::Instr_window() : Instr(PInstrCode::_window)
{
}

Instr_window::~Instr_window()
{
	for (Instr* const& instr : v_ww_instr) {
		delete instr;
	}
}

Instr_clrww::Instr_clrww() : Instr(PInstrCode::_clrww)
{
}

Instr_clrww::~Instr_clrww()
{
}

Instr_forall::Instr_forall() : Instr(PInstrCode::_forall)
{
}

Instr_forall::~Instr_forall()
{
	for (Instr* const& instr : CInstr) {
		delete instr;
	}
}

Instr_withshared::Instr_withshared(PInstrCode Kind) : Instr(Kind)
{
}

Instr_withshared::~Instr_withshared()
{
	for (Instr* const& instr : WDoInstr) {
		delete instr;
	}
	for (Instr* const& instr : WElseInstr) {
		delete instr;
	}

}

Instr_graph::Instr_graph() : Instr(PInstrCode::_graph)
{
}

Instr_graph::~Instr_graph()
{
}

Instr_putpixel::Instr_putpixel(PInstrCode Kind) : Instr(Kind)
{
}

Instr_putpixel::~Instr_putpixel()
{
}

Instr_backup::Instr_backup(PInstrCode Kind) : Instr(Kind)
{
}

Instr_backup::~Instr_backup()
{
}

Instr_closefds::Instr_closefds() : Instr(PInstrCode::_closefds)
{
}

Instr_closefds::~Instr_closefds()
{
}

Instr_setedittxt::Instr_setedittxt() : Instr(PInstrCode::_setedittxt)
{
}

Instr_setedittxt::~Instr_setedittxt()
{
}

Instr_setmouse::Instr_setmouse() : Instr(PInstrCode::_setmouse)
{
}

Instr_setmouse::~Instr_setmouse()
{
}

Instr_checkfile::Instr_checkfile() : Instr(PInstrCode::_checkfile)
{
}

Instr_checkfile::~Instr_checkfile()
{
}

Instr_login::Instr_login() : Instr(PInstrCode::_login)
{
}

Instr_login::~Instr_login()
{
}

Instr_sqlrdwrtxt::Instr_sqlrdwrtxt() : Instr(PInstrCode::_sqlrdwrtxt)
{
}

Instr_sqlrdwrtxt::~Instr_sqlrdwrtxt()
{
}

Instr_portout::Instr_portout() : Instr(PInstrCode::_portout)
{
}

Instr_portout::~Instr_portout()
{
}
