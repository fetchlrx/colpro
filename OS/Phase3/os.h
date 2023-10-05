#ifndef OS_H
#define OS_H

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <list>
#include <queue>
#include "defines.h"
/****************************************************************
 * Name(s): Jacob LeReaux 
 * File: OS.h
 *
 *
 ****************************************************************/
 
using namespace std;

class OS
{
   private:
      Assembler as;
      VirtualMachine vm;
      void checkwaitQ();
      int context_time;
      int idle_time;
      int system_time;
      int halt;
      double system_cpu;
      double user_cpu;   
      double throughput;
      string alg; //page replacement algorithm
      vector<int> inverted_page_table;
      double hitratio;   
      fstream progs_file; //file containing program list to run.
      string make_ofile(string sfilename);
      void load_process(string ofilename);
      
   public:
      //loads programs into cpu and initializes PCBs and Queues
      OS(char* progList = (char*)"progs", char* page_replacement_algorithm = (char*)"lru"); 
      ~OS();
      void syncVM(); //syncs PCB with vm before a program runs in virtual machine      
      void run();
      void syncPCB(); //syncs PCB with vm before context switch
      void context_switch(int status);
      void write(int reg);
      void read(int reg);
      void pagefault();
      void system_info(int level);
      int get_victim();
      void set_inverted(int frame, int p_id, int page);
      void reset_page_table_entry(int frame);
      int get_logical(int address_in_memory);
      void modify(int page);
      list <PCB*> jobs;
      queue <PCB*> readyQ, waitQ;
      queue <int> fifo;
      PCB * running;
      PCB * p;
};

#endif
