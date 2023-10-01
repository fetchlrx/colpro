#ifndef VIRTUAL_MACHINE_H
#define VIRTUAL_MACHINE_H

/***************************************************************
 * Name(s): Jacob Lereaux, Michael Kang 
 * File: VirtualMachine.h
 * Date: April 4, 2016 - April 18, 2016
 * Description: This code is an implementation of a 16-bit Virtual Machine (VM).
 * The VM reads the '.o' file from the Assembler and perform the following instruction
 * from the given instruction.
 ****************************************************************/

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <list>
#include <queue>
#include "PCB.h"
#include "defines.h"
#include "PageTable.h"

using namespace std;

class VirtualMachine 
{
private:
	vector<int> regs; //registers
	vector<int> mem; //main memory
	vector<int> lruFrame; //Frame Registers for lru page replacement algorithm
	
	instruction_register instr;
	int pc; //program counter
	int statusRegister; //status register
	int stackPointer; //stack pointer
	
	int limit; //program end
	int base;  //program start
	int clock; //clock
	int clock_start; //clock for cpu time for each program
	int hit;

	PageTable TLB; // Running Program's Page Table

	friend class OS;
	
    //masks for setting instruction register
	static const int op_mask = 63488; // 1111100000000000
	static const int rd_mask = 1536;  // 0000011000000000
	static const int i_mask = 256;    // 0000000100000000
	static const int rs_mask = 192;   // 0000000011000000
	static const int const_mask = 255;// 0000000011111111
	static const int addr_mask = 255; // 0000000011111111
	
	int convertAddressTLB(int logicalAddress);
	
	bool checkRegs(); //check range for register indexes
	bool checkConstant();  //check range for constant
	bool checkAddr();  //check range for address
	
    //functions for setting status register
	void setcarry(); //set carry bit of statusRegister
	void setgreater(); //set greater bit of statusRegister
	void setequal(); //set equal bit of statusRegister
	void setless(); //set less bit of statusRegister
	void setoverflow(int x, int y, int result); //checks sign of each. Set overflow bit accordingly
	void setStatus(int return_status); //puts the correct return status inside status register
	

    //functions for retrieving bits of status
	int carry(); //retrieve carry bit
	int is_greater(); //retrieve greater bit
	int is_equal(); //retrieve equal bit
	int is_less(); //retrieve less bit
	int overflow(); //retrieve overflow bit
	
	//Status Bits
	static const int timeslice = 0;
	static const int halt = 1;
	static const int out_bounds = 2;
	static const int stack_overflow = 3;
	static const int stack_underflow = 4;
	static const int invalid_instruction = 5;
	static const int read = 6;
	static const int write = 7;
	static const int page_fault = 32; //10 0000

public:
	VirtualMachine(); //constructor
	void load_page(fstream &objectfile, PCB* p, int page, int frame); //Loads programs to memory
	void run(PCB* running); //loads memory vector then performs instr-fetch-execute
	//void print() {
     	//for(int i = 0; i < 50; i++) //add5 + fact + test = 50 instructions
         //   cout << mem[i] << endl;
    	//}
		
};

#endif
