#ifndef PCB_H
#define PCB_H


/******************************
 * Name(s): Jacob Lereaux, Michael Kang
 * File: PCB.h
 * ****************************/
#include <vector>
#include <fstream>
#include <string>
#include "PageTable.h"
#include "defines.h"

using namespace std;

class PCB {
public:
    PCB();

    void set_process(string p, int id);
	void set_pagetable(string p);

	friend class OS;
	friend class VirtualMachine;
    
private:
    int pc;
    vector <int> regs;
	int statusRegister;
	int stackPointer;

    string process; //Name of the process
    int process_id;
    int base;
    int limit;
	
	int loadWordStoreWord; //used for load or store
	
    int wait_end;
	
    fstream* s,o,in,out,st;
	
    int cpu_time, wait_time, turnaround_time, io_time, largest_stack;
	
	int io_start, ready_start;	
	
	int page_fault_count;
	
	int stack_fault_count;
	
	PageTable pagetable;

};
#endif
