#include "PCB.h"

/******************************
 * Name(s): Jacob Lereaux, Michael Kang
 * File: PCB.cpp
 * ****************************/

PCB::PCB()
{
	pc = 0;
	regs = vector<int>(NUMBER_OF_REGISTERS);
	statusRegister = 0;
	stackPointer = 256;
	
	base = 0;
	limit = 8;
	process = "";
	
	loadWordStoreWord = 0;
	
	page_fault_count = 0;
	stack_fault_count = 0;
	largest_stack = 0;
	
	ready_start = 0;
	io_start = 0;
	
	cpu_time = 0;
	wait_time = 0;
	turnaround_time = 0;
	io_time = 0;
}
void PCB::set_process(string p, int id)
{
	process = p;
	process_id = id;
	set_pagetable(p);
}
void PCB::set_pagetable(string p)
{
	int lines = 0;
	string ofile = p + ".o";
	o.open(ofile.c_str(), ios::in | ios::out);
	while (o.good())
	{
		string ignore;
		getline(o, ignore);
		lines++; 
	}
	o.close();
	pagetable.set_size((lines/PAGE_SIZE)+1);
}
