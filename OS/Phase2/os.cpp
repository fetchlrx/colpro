#include "VirtualMachine.h"
#include "Assembler.h"
//#include <sys/types.h>
#include <unistd.h>
#include <string>
#include "PCB.h"
#include "os.h"
#include <sstream>




/****************************************************************
 * Name(s): Jacob Lereaux, Michael Kang 
 * File: os.cpp
 * Date: April 4, 2016 - April 18, 2016
 * Description: This code is an implementation of the Operating System (OS).
 * The os.cpp file brings together the code from the Assembler and Virtual Machine and test
 * weather it can properly link together the Asssmbler and Virtual Machine and perform
 * its task.
 ****************************************************************/

OS::OS(string arg)
{   
   context_time = 0; //time spent context switching
   idle_time = 0; //sum of all time not executing, and not context switching. Waiting for programs to complete their waitQ time. 
   system_time = 0; //sum of all idle_time and context_switch times
   system_cpu = 0; 
   user_cpu = 0;   
   throughput = 0; //
   halt = 0; //number of programs that have halted

   //Gathers all *.s files and puts them into a file called "progs"
   system("ls progs/*.s > progsList");

   //Init of variables and objects
   fstream assemblefile, objectfile, progs_file;
   string sfilename, ofilename;

   //Open "progs" file so program can grab the list of *.s files to
   //assemble. Then input file stream into a string. inputfile will
   //contain add5.s then fact.s and then test.s
   progs_file.open("progsList", ios::in);
   progs_file >> sfilename;
    
   p = new PCB;
   //The while loops loops through all files in "progs" and assemble their 
   //*.o file.
   while(progs_file.good()) 
   {
      //First get process name
      p->set_process(sfilename.substr(0, sfilename.length() - 2));
      
      assemblefile.open(sfilename.c_str(), ios::in);
   
      ofilename = sfilename.replace(sfilename.find('.'), sfilename.length(),".o");
      objectfile.open(ofilename.c_str(),ios::out);
   
      //Assemble the files to .o
      as.assemble(assemblefile, objectfile);
      
      //Close i/o stream
      objectfile.close();
      assemblefile.close();

      //Load each object program into memory and set up process info
      objectfile.open(ofilename.c_str(), ios::in);
      
      //////////////VM LOAD/////////////////
        vm.load(objectfile, p);
      ///////////////////////////////////

      objectfile.close();

      //push processor into jobs list
      jobs.push_back(p);

      //create a new object of type PCB
      p = new PCB;

      //input next .s file into inputfile
      progs_file >> sfilename;
    }
    progs_file.close(); //close file w/ list of progs


   //push jobs into readyQ
   list<PCB*>::iterator it = jobs.begin();
   for(it; it != jobs.end(); it++)
   {
         readyQ.push(*it);   
      (*it)->ready_start = vm.clock; 
   }

    //open all output streams
    for(it = jobs.begin(); it != jobs.end(); it++)
   {
      string outfile = (*it)->process + ".out";
      (*it)->out.open(outfile.c_str(), ios::out);

      string infile = (*it)->process + ".in";
      (*it)->in.open(infile.c_str(), ios::in);
   }
   if(DEBUG > 0)
   { //DEBUG defined in VirtualMachine.h
      cout << "------PROCESSES---------" << endl;
      int i = 1;
      for(it = jobs.begin(); it != jobs.end(); it++)
      {
         cout << i << ": " << (*it)->process << endl; 
         cout << "  base: " << (*it)->base << endl;
         cout << "  limit: " << (*it)->base << endl;
         i++;
      }
      cout << "---------MEMORY AT EXECUTION START----------" << endl;
      for (int i = 0; i < vm.mem.size(); i++){
         cout << i << ": " << vm.mem[i] << endl;
      }
      cout << endl;
      
   }
}


void OS::run()
{
   running = readyQ.front(); //set running pointer to first element in readyQ
   int incr = 0;
   while (running != NULL) //while there are programs to run, run
   {
      if(readyQ.size() > 0)
      {
         running = readyQ.front(); //load program to execute
         readyQ.pop(); //take it out of readyQ
         running->wait_time += (vm.clock - running->ready_start); //calculate time program spent waiting in readyQ to run
      }

      syncVM(); //VM values = PCB values
      vm.run(running); //run program
      running->cpu_time += (vm.clock - vm.clock_start); //add different of clock start and clock to cpu time
      syncPCB(); //PCB values = VM values
      context(vm.sr); //context switch
      if(readyQ.size() == 0 && waitQ.size() == 0) // if both queues are empty
      {  
         running = NULL;
      }
   }//end while

   //accounting   
   list<PCB*>::iterator it = jobs.begin();
   for(it; it != jobs.end(); it++) 
   {
      user_cpu += (*it)->cpu_time;
   }
   user_cpu = (user_cpu / (double)vm.clock) * 100;
   system_time = context_time + idle_time; //time of all context switches
   system_cpu = ( ((double)vm.clock - (double)idle_time) / (double)vm.clock ) * 100.00;
   cout << vm.clock << endl;
   throughput = (double)jobs.size() / ((double)vm.clock/(double)1000);

   for(it = jobs.begin(); it != jobs.end(); it++) 
   {
      (*it)->out << "System Time: " << system_time << endl;
      (*it)->out << "System CPU Utilization:" << system_cpu << endl;
      (*it)->out << "User CPU Utilization: " << user_cpu << endl;
      (*it)->out << "Throughput: " << throughput << endl;      
      (*it)->out.close();
      (*it)->in.close();
   }
   return;
}
void OS::context(int status)
{
   checkwaitQ();

   status >>= 5;
   int reg = status >> 3;
   status &= 7;
   if(status == 0) //timeslice status
   {   
      running->ready_start = vm.clock; //start wait time
      readyQ.push(running); //move running program to readyQ
   }
   else if(status == 1)//halt (write clock to outfile
   {
      halt++;
      running->turnaround_time = vm.clock;
      running->out << "------Accounting Information------ " << endl;
      running->out << "CPU: " << running->cpu_time << endl;
      running->out << "Waiting Time: " << running->wait_time << endl;
      running->out << "Turnaround: " << running->turnaround_time << endl;
      running->out << "I/O Time: " << running->io_time << endl;
      running->out << "Largest Stack Size: " <<  running->lstack_size << endl;
      running->out << endl;
   }
   else if(status == 6) //read status 
   {
      running->wait = vm.clock+27;
      running->io_start = vm.clock; //start io time
      waitQ.push(running); /// move program to waitQ
      read(reg); //call read()
   }
   else if(status == 7) //write status
   {
      running->wait = vm.clock+27; 
      running->io_start = vm.clock; //start io time
      waitQ.push(running); /// move program to waitQ
      write(reg); //call write()
   }
   if(readyQ.empty() && !waitQ.empty())
   {
      idle_time += waitQ.front()->wait - vm.clock;
      vm.clock += waitQ.front()->wait - vm.clock;
      checkwaitQ();
   }
   context_time += 5;
   vm.clock += 5;
   return;
}
void OS::syncVM()
{
   //set all vm values from values inside PCB
   vm.base = running->base;
   vm.limit = running->limit;
   vm.pc = running->pc;
   vm.sp = running->sp;
   vm.sr = running->sr;
   vm.r = running->r;
   if(running->sp < vm.MEM_SIZE) //there is a stack   
   {   
      //open stackfile
      string stackfile = running->process + ".st";
      running->st.open(stackfile.c_str(),ios::in);
      
      int i = vm.MEM_SIZE;
      while(i > vm.sp)
      {
          //starting from the bottom right from stackfile into memory
          running->st >> vm.mem[--i];
      }
      running->st.close();
   }
   
   return;
}
void OS::syncPCB()
{
   //set all values inside pcb from values inside vm
   running->base = vm.base;
   running->limit = vm.limit;
   running->pc = vm.pc;
   //check size of stack for largest stack size
   if((vm.MEM_SIZE - vm.sp) > running->lstack_size) //if virtual machine contains a larger stack than the PCB
   {   
      running->lstack_size = (vm.MEM_SIZE - vm.sp); //largest stack size = stack 
   }
   running->sp = vm.sp;
   running->sr = vm.sr;
   running->r = vm.r;
   if(vm.sp < vm.MEM_SIZE) //there is a stack
   {   
      //open stack file
      string stackfile = running->process + ".st";
      running->st.open(stackfile.c_str(), ios::out);
      //i is the very bottom
      int i = vm.MEM_SIZE;
      while(i > vm.sp)
      {
         //starting from the bottom, write whats inside mem into stack file
         running->st << vm.mem[--i] << endl;
      }   
      running->st.close();
   }
   return;
}
void OS::read(int reg)
{
   //read from write file and place it inside r[reg] of PCB
   running->in >> running->r[reg];
   return;
}
void OS::write(int reg)
{
   //open readfile
   //write r[reg] inside PCB
   if(running->r[reg] > 32767) //negative values
   {   
      int temp;
         temp = running->r[reg] | 4294901760; //sign extend to 32 bits
       running->out << "Output: " << temp << endl;
   }   
   else //positive values
   {
      running->out << "Output: " << running->r[reg] << endl;
   }
   return;
}
void OS::checkwaitQ()
{   if(!waitQ.empty())
   {
      //if I/O complete
      if((vm.clock+5) >= (waitQ.front()->wait))
      {
         //move to readyQ
         readyQ.push(waitQ.front());
         //pop waitQ
         waitQ.pop();
         //stop io time
         readyQ.back()->io_time += (vm.clock - readyQ.back()->io_start); //add the difference of clock and original io_time
         //start wait time      
         readyQ.back()->ready_start = vm.clock; 
      }
   }
   return;
}

int main(int argc, char *argv[])
{
   //Keeping use of args by casting method
   argv[1] = (char *) "progs";
   string arg = argv[1];

   //Instantiate OS//
   OS os(arg);

   //run os//
   os.run();

   return 0;
}//main
