#include "Instr.h"

Instr_menu::Instr_menu(PInstrCode Kind)
{
	this->Kind = Kind;
}

Instr_loops::Instr_loops(PInstrCode Kind)
{
	this->Kind = Kind;
}

Instr_merge_display::Instr_merge_display(PInstrCode Kind)
{
	this->Kind = Kind;
}

Instr_proc::Instr_proc(size_t TArg_Count)
{
	Kind = PInstrCode::_proc;
}

Instr_lproc::Instr_lproc()
{
	Kind = PInstrCode::_lproc;
}

Instr_call::Instr_call()
{
	Kind = PInstrCode::_call;
}

Instr_exec::Instr_exec()
{
	Kind = PInstrCode::_exec;
}

Instr_copyfile::Instr_copyfile()
{
	Kind = PInstrCode::_copyfile;
}

Instr_writeln::Instr_writeln()
{
	Kind = PInstrCode::_writeln;
}

Instr_gotoxy::Instr_gotoxy()
{
	Kind = PInstrCode::_gotoxy;
}

Instr_assign::Instr_assign(PInstrCode Kind)
{
	this->Kind = Kind;
}

Instr_help::Instr_help()
{
	Kind = PInstrCode::_help;
}

Instr_recs::Instr_recs(PInstrCode Kind)
{
	this->Kind = Kind;
}

Instr_turncat::Instr_turncat()
{
	Kind = PInstrCode::_turncat;
}

Instr_sort::Instr_sort()
{
	Kind = PInstrCode::_sort;
	SK = new KeyFldD();
}

Instr_sort::~Instr_sort()
{
	delete SK;
}

Instr_edit::Instr_edit()
{
	Kind = PInstrCode::_edit;
}

Instr_report::Instr_report()
{
	Kind = PInstrCode::_report;
}

Instr_edittxt::Instr_edittxt(PInstrCode Kind)
{
	this->Kind = Kind;
}

Instr_puttxt::Instr_puttxt()
{
	Kind = PInstrCode::_puttxt;
}

Instr_releasedrive::Instr_releasedrive()
{
	Kind = PInstrCode::_releasedrive;
}

Instr_mount::Instr_mount()
{
	Kind = PInstrCode::_mount;
}

Instr_indexfile::Instr_indexfile()
{
	Kind = PInstrCode::_indexfile;
}

Instr_getindex::Instr_getindex()
{
	Kind = PInstrCode::_getindex;
}

