#include "PCB.h"

void PCB::set_data(int pc, int base, int limit, int sr, vector<int> r, int sp)
{
	set_pc(pc);
	set_base(base);
	set_limit(limit);
	set_sr(sr);
	set_r(r);
	set_sp(sp);
}
