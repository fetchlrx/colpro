#ifndef OS_H
#define OS_H
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <list>
#include <queue>

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
   public:
      OS(string arg); //loads programs into cpu and initilializes PCBs and Queues
      void syncVM(); //syncs PCB with vm before context switch      
      void run();
      void syncPCB(); //syncs PCB with vm before context switch
      void context(int status);
      void write(int reg);
      void read(int reg);
      void accounting();
      list <PCB*> jobs;
      queue <PCB*> readyQ, waitQ;   
      PCB * running;
      PCB * p;
};

#endif
