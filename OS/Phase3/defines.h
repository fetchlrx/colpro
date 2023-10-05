/*******************************************************************************************
 * File: defines.h
 *
 * Name(s): Jacob LeReaux
 *
 * Last Modified: March 2018
 * 
 * Description: This file will contain Macros used by the Virtual Machine, Assembler, & OS
 *
 *******************************************************************************************/

#define DEBUG 0
#define NUMBER_OF_REGISTERS 4 //number of registers
#define MEM_SIZE 256  //size of main memory
#define FRAME_SIZE 8
#define PAGE_SIZE FRAME_SIZE
#define MAX_PAGE_BITS 10 //1024 Max Pages
#define NUM_PAGES 1024
#define MAX_PAGE NUM_PAGES-1
#define STACK_PAGE_ID MAX_PAGE
#define STACK_NUM_WORDS 6 //number of words to save during a stack call

//for system_info print routine
#define SYS_INFO_PROCESS_HALT 1
#define SYS_INFO_OS_HALT -1


#ifndef DEFINES_H
#define DEFINES_H

struct instruction_register{
      int op; //operand;
      int destReg; //index for destination register
      int immediate;  //immediate
      int sourceReg; //index source register
      int constant; //constant
      int addr; //address
};

#endif