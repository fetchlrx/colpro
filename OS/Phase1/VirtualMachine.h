#ifndef VIRTUAL_MACHINE_H
#define VIRTUAL_MACHINE_H

/***************************************************************
 * Name(s): Jacob LeReaux, Michael Kang 
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

using namespace std;

class VirtualMachine 
{
private:
    static const int REG_FILE_SIZE = 4; //number of registers
	static const int MEM_SIZE = 256; //size of main memory
	vector<int> r; //registers
	vector<int> mem; //main memory
	int pc; //program counter
	int ir; //instruction register
	int sr; //status register
	int sp; //stack pointer
	int limit; //program end
	int base;  //program start
	int clock; //clock
	int op; //operand;
	int rd; //index for destination register
	int i;  //immediate
	int rs; //index source register
	int con; //constant
	int addr; //address
	
    //masks for setting instruction register
	static const int op_mask = 63488; // 1111100000000000
	static const int i_mask = 256;    // 0000000100000000
	static const int rd_mask = 1536;  // 0000011000000000
	static const int rs_mask = 192;   // 0000000011000000
	static const int const_mask = 255;// 0000000011111111
	static const int addr_mask = 255; // 0000000011111111
	
    //functions to check range of instruction
	void checkrd(); //check range for destination register
	void checkrs();  //check range source register
	void checkcon();  //check range for constant
	void checkaddr();  //check range for address
	
    //functions for status register
	void setcarry(); //set carry bit of sr
	void setgreater(); //set greater bit of sr
	void setequal(); //set equal bit of sr
	void setless(); //set less bit of sr
	void setoverflow(int x, int y, int result); //checks sign of each. Set overflow bit accordinly
	
    //functions for retrieving bits of status
	int carry(); //retrieve carry bit
	int greater(); //retrieve greater bit
	int equal(); //retrieve equal bit
	int less(); //retrieve less bit
	int overflow(); //retrieve overflow bit
public:
	VirtualMachine(); //constructor
	void run(fstream &object, fstream &input, fstream &output); //loads memory vector then performs instr-fetch-execute
};

#endif
