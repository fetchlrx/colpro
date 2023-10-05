#ifndef PCB_H
#define PCB_H


/******************************
 *
 *
 * ****************************/
#include <vector>
#include <fstream>
#include <string>

using namespace std;

class PCB {
public:
    PCB(): pc(0), sr(0), base(0), limit(0), sp(256), process(""), cpu_time(0), wait_time(0), io_start(0), turnaround_time(0), io_time(0), lstack_size(0), ready_start(0){ }
    ~PCB() { delete this; };
    
    void set_data(int pc, int base, int limit, int sr, vector<int> r, int sp);
    
    //set process name
    void set_process(string p) {process = p;}
   
    //Getter methods
    int get_pc() { return pc; }
    int get_sr() { return sr; }
    int get_sp() { return sp; }
    vector<int> get_r() { return r; }
    int get_base() { return base; }
    int get_limit() { return limit;}
    string get_process() { return process; }


   friend class OS;
    
private:
    int pc; //program counter
    int sr; //status register
    vector <int> r; //registers
    string process; //Name of the process
    int base, limit; //bounds in memory for program
    int sp; //stack pointer for this process
    int wait;
    fstream* s,o,in,out,st; //streams for each program
    int cpu_time; //time spent executing
   int wait_time; //time spent in readyQ, waiting to execute
   int turnaround_time; //total time from start to finish
   int io_time; //time spent in waitQ, waiting for IO to complete
   int lstack_size; //largest stack size for program
   int io_start, ready_start; //times entering waitQ and ReadyQ respectively

    //Setter methods
    void set_pc(int p) { pc = p; }
    void set_sr(int s) { sr = s; }
    void set_sp(int s) { sp = s; }
    void set_r(vector <int> reg) { r = reg; }
    void set_base(int b) { base = b; }
    void set_limit(int l) { limit = l; }
    
    //Below should have accounting info
    friend class VirtualMachine;
};

#endif
