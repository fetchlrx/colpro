/*******************************************************************************************
 * File: os.cpp
 *
 * Name(s): Jacob LeReaux
 *
 * Last Modified: March 2018
 * 
 * Description: This Code is an implementation of a low level, 16-bit Operating System.
 * 	This OS has multi-programming ability, as well as paging ability. During initialization,
 * 	ths OS will take a list of processes listed in a program file. It will command the VM to 
 * 	one-by-one load the first page of the first five processes into memory. We do this by 
 *	a process written in simplified Assembly and assemble each written instruction into a 
 *	respective integer, that represents the entire instruction. Once the programs are loaded
 *	into memory, we can begin by adding all processes to the Ready Queue and to begin
 *	execution. The VM will be responsible for keeping track of the time spent in the VM and
 *	giving control back to the OS once a program reaches the specified timeslice. The OS 
 *	will also gain control whenever an IO request is made, or if a page fault occurs.
 *	The OS has an inverted page table to keep track of which process owns each frame, as
 *	well as which page. The OS will be responsible for context switching accordingly and 
 * 	to make sure each program is in its respective queue (Ready or Wait), at the time of a 
 *	context switch. During the execution of this OS, the addresses stored inside of a PCB
 *	will be the logical addresses, while, when inside of the VM, will be the physical
 *	addresses. The OS is also responsible for writing any data to the hard disk that a 
 *	program modifies. During execution, the OS will be calculating system information and 
 *	write the information to each program's output file at the end of execution. Once
 * 	a program successfully halts, it will load a new process into memory as well as
 *	create the program's PCB. If stack is written during the execution of a program, the OS
 * 	will need to update the inverted page table accordingly to specify which frames are used
 *	by stack. Currently, the OS will write stack data to a .st file during context switches.
 *  As well as the .st file, each program will have one .s, .o, .out, and one .in file. .s 
 *	files contain Assembly instructions. .o files are the Assembled integer instructions. 
 *	.out files store any output from a program. Lastly, .in files will store any input 
 *	needed by a program. The OS will context switch using a round-robin implementation, 
 *	while all processes currently have the same priority, meaning there is no preemption. 
 *
 *******************************************************************************************/
#include "VirtualMachine.h"
#include "Assembler.h"
#include <unistd.h>
#include <string>
#include "PCB.h"
#include "defines.h"
#include "os.h"
#include <sstream>
#include <string.h>

OS::OS(char* progList, char* page_replacement_algorithm)
{
	context_time = 0;
	idle_time = 0;
	system_time = 0;
	system_cpu = 0;
	user_cpu = 0;
	throughput = 0;
	halt = 0;
	inverted_page_table = vector<int>((MEM_SIZE/FRAME_SIZE),0);
	hitratio = 0;
	
	if(strncmp(page_replacement_algorithm, (char*)"lru", 3) == 0)
	{
		alg = "lru";
	}
	else if(strncmp(page_replacement_algorithm, (char*)"fifo", 4) == 0)
	{
		alg = "fifo";
	}
	else
	{
		cout << "Unrecognized Command" << endl;
		exit(0);
	}
	if (DEBUG > 0){
		cout << "page replacement algorithm = " << alg << endl;
	}
	
	//Gathers all *.s files and puts them into a file called "progs"
    system("ls progs/*.s > progsList");
	
    progs_file.open(progList, ios::in);
    //The while loops loops through all files in "progs" and assemble their 
    //*.o file and also loads them into memory with new PCB for each
	while(jobs.size() < 5 && progs_file.good()) 
	{
		string sfilename; //declaration
		
		//put next program in progs_file into sfilename
		progs_file >> sfilename;
		
		//Make .o file and place the string in ofilename
		string ofilename = make_ofile(sfilename);
		
		//load process and create new PCB
		load_process(ofilename);
		
        //push processor into jobs list
        jobs.push_back(p);
	}	

	//push jobs into readyQ
    list<PCB*>::iterator it = jobs.begin();
    for(it; it != jobs.end(); it++)
	{
	   	readyQ.push(*it);	
		(*it)->ready_start = vm.clock; 
	}
	
    //open streams .o file, .out (output) file, and .in (input file)
    for(it = jobs.begin(); it != jobs.end(); it++)
	{
		string ofile = (*it)->process + ".o";
		(*it)->o.open(ofile.c_str(), ios::in | ios::out);

		string outfile = (*it)->process + ".out";
		(*it)->out.open(outfile.c_str(), ios::out);

		string infile = (*it)->process + ".in";
		(*it)->in.open(infile.c_str(), ios::in);

	}
	
	//Debugging information
	if(DEBUG > 1){
		cout << "-------------------------------------------------" << endl;
		cout << "--------------inverted_page_table----------------" << endl;
		for(int i = 0; i < inverted_page_table.size(); i++)
		{ 
			int p = (inverted_page_table[i] >> (MAX_PAGE_BITS + 1));
			int pa = ((inverted_page_table[i] >> 1) & 1023);
			cout << "Frame " << i << " = " << p << " " << pa << endl;
		}
	}
}
OS::~OS()
{
	list<PCB* >::iterator it;
	for(it = jobs.begin(); it != jobs.end(); it++) {
		delete *it;
	}
}

/*///////////////////////////////////////////////////////////////////////////////////////////////
*
* Function: make_ofile
*
* Description: assembles the program file (in assembly) into a .ofile (binary instructions)
*
* Inputs: string filename - filename that we want to assemble
*
* Outputs: returns: string ofilename - the name of the .o file that was created
*
/*///////////////////////////////////////////////////////////////////////////////////////////////
string OS::make_ofile(string sfilename){
	
	fstream assemblefile, objectfile; //create streams
	
	assemblefile.open(sfilename.c_str(), ios::in); //open the .s file
	
	string ofilename = sfilename.replace(sfilename.find('.'), sfilename.length(),".o");//.o file
	
	objectfile.open(ofilename.c_str(),ios::out); //open the .o file
	
	////////Assemble the .s file into the .o file//////////////
	as.assemble(assemblefile, objectfile); 

	//close streams
	assemblefile.close();
	objectfile.close();
	
	return ofilename;
}

/*///////////////////////////////////////////////////////////////////////////////////////////////
*
* Function: load_process
*
* Description: loads the first page of the process we are going to run into RAM. 
*			Creates new Program Control Block for the new process.
*			Sets inverted page table 
*
* Inputs: string ofilename - .o filename that contains the instructions we want to load in memory
*
* Outputs: none
*
/*///////////////////////////////////////////////////////////////////////////////////////////////
void OS::load_process(string ofilename){
	static int process_count = 1; //number of processes loaded

	p = new PCB; //create a new object of type PCB
	p->set_process(ofilename.substr(0, ofilename.length() - 2), process_count);
	process_count++;
        
	fstream objectfile;
	objectfile.open(ofilename.c_str(), ios::in);
	
	//load very first program of every file into memory
	const int page1 = 0; //get the first page of each program
	
	int victim = get_victim();	//frame to be replaced
	
	vm.load_page(objectfile, p, page1, victim);	//load the page into the appropriate frame
	
	set_inverted(victim, p->process_id, page1);
	
    //Close stream
    objectfile.close();
}

/*///////////////////////////////////////////////////////////////////////////////////////////////
*
* Function: run
*
* Description: Starts the execution of the OS.
*
* Inputs: none
*
* Outputs: none
*
/*///////////////////////////////////////////////////////////////////////////////////////////////
void OS::run()
{
	running = readyQ.front(); 
	//Main Scheduling Loop
	while (running != NULL)
	{
		if(readyQ.size() > 0)
		{
			running = readyQ.front(); //load program to execute
			readyQ.pop(); //take the now running process out of the readyQ
			
			//add the amount waited in readyQ to the total
			running->wait_time += (vm.clock - running->ready_start); 
		}
		
		int page = (running->pc / PAGE_SIZE);
		///////////if page is loaded into memory, then run the VM/////////////////
		if(running->pagetable.get_valid(page) == true)
		{
			syncVM(); //VM values = PCB values
			
			//////// RUN PROGRAM ////////
			vm.run(running); 
			//accounting information
			//add difference of clock start and clock to cpu time
			running->cpu_time += (vm.clock - vm.clock_start);
			
			syncPCB(); //PCB values = VM values
		}
		else //page is not loaded into memory. Treat it like a page fault and context switch
		{
			//Set status registers. All other registers do not need to change 
			vm.setStatus(vm.page_fault);//Set Virtual Machine Status Register for Context Switch
			running->statusRegister = vm.statusRegister; //Set PCB Status Register.
		}//////////////////////////////////////////////////////////////////////////////
		
		/////// CONTEXT SWITCH ////////
		context_switch(vm.statusRegister);
		///////////////////////////////
		
		if(readyQ.size() == 0 && waitQ.size() == 0) // if both queues are empty
		{  
			running = NULL; //set running to NULL to end the while loop
		}
	}//end while

	/////////////////////// Accounting Information for Each Program. /////////////////////// 
	system_info(SYS_INFO_OS_HALT);
	
	return;
}

/*///////////////////////////////////////////////////////////////////////////////////////////////
*
* Function: context_switch
*
* Description: performs the context switch when updating ready and wait queues
*
* Inputs: int status - status register from vm
*
* Outputs: none
*
/*///////////////////////////////////////////////////////////////////////////////////////////////
void OS::context_switch(int status)
{
	checkwaitQ();

	status >>= 5;
	int p_fault = status >> 5; //page_fault
	int ioReg = status >> 3;
	status &= 7;

	if(status == 0 && p_fault == 0) //timeslice status
	{	
		running->ready_start = vm.clock; //start wait time
		readyQ.push(running); //move running program to readyQ
	}
	else if(status == 0 && p_fault == 1) //PAGE_FAULT
	{ 
		running->page_fault_count++;//add accounting, other stuff//////////
		running->wait_end = vm.clock+35;
		running->io_start = vm.clock; //timestamp entering waitQ
		waitQ.push(running);

		int page;
		
		if(running->loadWordStoreWord == 0) //if page fault due to PC increment or jump or call
		{	
			page = running->pc/PAGE_SIZE; //page
		}
		else //if page fault due to a load or store operation
		{
			//page for load word or store word
			page = running->loadWordStoreWord/PAGE_SIZE; 
		}
		if(DEBUG > 0)
		{
			cout << "PAGE FAULT. Process: " << running->process << endl;
			cout << "        PAGE: " <<  page << endl;
			cout << endl;
		}
		int victim = get_victim();//frame to be replaced
		vm.load_page(running->o, running, page, victim);//load the page into the appropriate frame
		set_inverted(victim, running->process_id, page);
	
		running->loadWordStoreWord = 0; //reset loadWordStoreWord
	}
	else if(status == 1)// 1 = successful halt.
	{ 
		if(DEBUG > 0){
			cout << "SUCCESSFULLY HALTING PROGRAM: " << running->process << endl;
		}
		halt++;
		running->turnaround_time = vm.clock;
		
		system_info(SYS_INFO_PROCESS_HALT);
		
		string sfilename;
		
		progs_file >> sfilename;
		
		if(!sfilename.empty()) //load new program
		{
			//make .o file from .s file and store the string of the .o file.
			string ofilename = make_ofile(sfilename);
			
			//load the new programs first page of instructions into memory
			load_process(ofilename);
			
		    //push processor into jobs list
		    jobs.push_back(p);

			//push process onto ready queue
			readyQ.push(p);
			p->ready_start = vm.clock; //timestamp entering ready queue
	
			//Open Streams: .o file(instructions), .out file (output), .in file (input)
			string ofile = p->process + ".o";
			p->o.open(ofile.c_str(), ios::in | ios::out);

			string outfile = p->process + ".out";
			p->out.open(outfile.c_str(), ios::out);

			string infile = p->process + ".in";
			p->in.open(infile.c_str(), ios::in);
		}
	}
	else if(status == 3 && p_fault == 1)
	{
		// stack fault - a page must be taken out of memory 
		// before we write stack.
		
		//add for process information
		running->stack_fault_count++;
		
		//add to wait queue with a time limit 
		// and timestamp when we enter.
		running->wait_end = vm.clock+35;
		running->io_start = vm.clock;
		waitQ.push(running);
		
		
		//Kick out whichever frame we need to kick out.
		int frame_to_kick_out = (vm.stackPointer - STACK_NUM_WORDS) / FRAME_SIZE;
		int process_id_of_frame = inverted_page_table[frame_to_kick_out] >> (MAX_PAGE_BITS + 1);
		int page_num = inverted_page_table[frame_to_kick_out] >> 1;
		page_num &= (MAX_PAGE); 
		
		
		//If the modify bit is high for this frame, write it to hard disk
		if(process_id_of_frame == running->process_id)
		{
			if(running->pagetable.get_mod(page_num))
			{
				if(DEBUG > 0)
				{
					cout << "MODIFY PAGE:" << page_num << " for PROCESS: ";
					cout << running->process << ", due to stack overflow" << endl;
				}
				modify(page_num);
			}
		}
		
		if((inverted_page_table[frame_to_kick_out] >> 1) != 0)
		{ //some program is using this frame
			//reset the page table entry that has this frame
			reset_page_table_entry(frame_to_kick_out);
			//set the inverted page table to stack
			set_inverted(frame_to_kick_out, 0, 0); 
		}
		else
		{
			//if the frame is free, reserve it for stack
			inverted_page_table[frame_to_kick_out] = 1;
		}
		
		//update lruFrame
		vm.lruFrame[frame_to_kick_out] = vm.clock;
		
		if(DEBUG > 0)
		{
			cout << "STACK FAULT for PROCESS: " << running->process << endl;
			cout << "Kicking page: " << page_num << " out of frame: " << frame_to_kick_out << endl;
		}
	}
	else if((status == 3 || status == 4) && p_fault == 0)
	{
		string errorMsg = "";
		if(status == 3)
			errorMsg = "OverFlow";
		else
			errorMsg = "UnderFlow";
		
		//stack overflow or underflow. 
		//Doing nothing will cause the program to suspend.
		cout << "Stack " << errorMsg << " for Process: ";
		cout << running->process << endl;
		cout << "		-- Suspending Process --" << endl;
		
	}
	else if(status == 6 || status == 7) //IO call  
	{
		running->wait_end = vm.clock+27; //timestamp leaving ready queue
		running->io_start = vm.clock; //timestamp entering wait Q
		waitQ.push(running); // move program to waitQ
		if(status == 6){
			read(ioReg); //call read()
			if(DEBUG > 0)
			{
				cout << "READ CALL TO OS" << endl;
			}
		}
		else if(status == 7){
			write(ioReg); //call write()
			if(DEBUG > 0)
			{
				cout << "WRITE CALL TO OS" << endl;
			}
		}
	}
	else //error
	{
		cout << "Incorrect Status Code for Process: " << running->process << ". STATUS: " << status << endl;
	}
	
	//Kick all the pages out of memory that were used for stack.
	if(vm.stackPointer < MEM_SIZE)
	{ // Stack was used. We need to kick whichever pages were in those frames.
		int currentFrame = vm.stackPointer / FRAME_SIZE;
		for(int i = currentFrame; i < (MEM_SIZE / FRAME_SIZE); i++)
		{
			if(DEBUG > 0)
			{
				cout << "FREEING FRAME: " << currentFrame << " FOR STACK" << endl;
				cout << "Inverted Page Table: " << (inverted_page_table[currentFrame] >> (MAX_PAGE_BITS + 1));
				cout << ", " << ((inverted_page_table[currentFrame] >> 1) & MAX_PAGE)  << endl;
				for(int j = 0; j < FRAME_SIZE; j++)
				{
					cout << (currentFrame * FRAME_SIZE + j) << ": " << vm.mem[(currentFrame * FRAME_SIZE + j)] << endl;
				}
			}
			
			//stack is process 0, page 0. If it is not already used by stack
			if((inverted_page_table[currentFrame] >> 1) != 0)
			{ // then some program is using this frame
				
				//reset the page table entry inside of whichever process has this frame
				reset_page_table_entry(currentFrame);
				//set the inverted page table to stack
				set_inverted(currentFrame, 0, 0); 
			}
			else
			{
				//if this is a free frame, reserved it for stack.
				inverted_page_table[currentFrame] = 1;
			}
			
			//update lru register
			vm.lruFrame[currentFrame] = vm.clock;
			currentFrame++;
		}
	}
	
	if(readyQ.empty() && !waitQ.empty())
	{
		//there is nothing in ready queue, but there are programs in wait queue
		//if so, add the time remaining to idle_time and the vm_clock, so that 
		//the time is up and the program can be placed in ready queue. 
		//Essentially we are speeding up the clock because there is nothing else to do
		idle_time += waitQ.front()->wait_end - vm.clock;
		vm.clock += waitQ.front()->wait_end - vm.clock;
		checkwaitQ(); //check the wait queue again
	}
	
	context_time += 5;
	vm.clock += 5;
	return;
}

/*///////////////////////////////////////////////////////////////////////////////////////////////
*
* Function: syncVM
*
* Description: sets all registers and stack in Virtual Machine to those of the 
*			about-to-run process
*
* Inputs: none
*
* Outputs: none
*
/*///////////////////////////////////////////////////////////////////////////////////////////////
void OS::syncVM()
{
	//Set all values in vm to what 
	int page = (running->pc/PAGE_SIZE);
	vm.base = (running->pagetable.get_frame(page)) * PAGE_SIZE;// get the frame from page table
	vm.pc = vm.base + (running->pc % PAGE_SIZE);
	vm.limit = vm.base + PAGE_SIZE;
	vm.stackPointer = running->stackPointer;
	vm.statusRegister = running->statusRegister;
	vm.regs = running->regs;
	vm.TLB = running->pagetable;
	
	
	int frame = vm.pc / FRAME_SIZE;
	
	if(vm.pc > vm.stackPointer)
	{ //kick the page out of memory
		reset_page_table_entry(frame); //reset the page table entry that has this frame
		inverted_page_table[frame] = 1; //reserve the frame for stack
		vm.lruFrame[frame] = vm.clock;
	}
	
	if(running->stackPointer < MEM_SIZE) //there is a stack	
	{
		//open stackfile
		string stackfile = running->process + ".st";
		running->st.open(stackfile.c_str(),ios::in);
		
		int i = MEM_SIZE;
		while(i > vm.stackPointer)
		{
			 //starting from the bottom right from stackfile into memory
			 running->st >> vm.mem[--i];
		}
		running->st.close();
	}
	
	return;
}

/*///////////////////////////////////////////////////////////////////////////////////////////////
*
* Function: syncPCB
*
* Description: sets all registers and stack in the PCB that previously ran to those of the 
*			Virtual Machine to store for the next time the program runs
*
* Inputs: none
*
* Outputs: none
*
/*///////////////////////////////////////////////////////////////////////////////////////////////
void OS::syncPCB()
{
	//We do not care about base and limit registers inside of the PCB
	//because we will reevaluate those before going into the VM based on the logical pc address.
	//We will convert the logical pc to the physical pc and reevaluate those registers in the VM
	
	//If no page fault, we must calculate the logical address using the inverted page table
	if(vm.pc < vm.limit && vm.pc >= vm.base) //no page fault
	{
		running->pc = get_logical(vm.pc);
	}
	//If it is an incremental pagefault, we will not have that page loaded into memory.
	//Find the address before this one because we know this address is currently in memory. 
	//Then Add one
	else if(vm.pc == vm.limit) //incremental page fault
	{	
		//go to previous address because the current one is not loaded into memory
		int logical_address = get_logical(vm.pc-1);
		if(logical_address >= 0)
		{
			//Store logical address of the program counter
			running->pc = logical_address + 1; 
		}
	}
	//Call or jump addresses will be logical addresses even inside the VM.
	else //call or jump address
	{
		running->pc = vm.pc; //calls and jumps point to logical address 
	}
	running->pagetable = vm.TLB;

	running->base = (running->pc / PAGE_SIZE) * PAGE_SIZE;
	
	for(int i = 0; i < running->pagetable.table.size(); i++)
	{
		if(running->pagetable.get_mod(i))
		{
			if(DEBUG > 0){
				cout << "MODIFY PAGE:" << i << " for PROCESS: " << running->process << endl;
			}
			modify(i);
		}
	}
	
	if(DEBUG > 1){
		cout << "---------------------------------" << endl;
		cout << "Context Switching " << running->process << endl;
		cout << "PC VM -> PCB" << vm.pc << " -> " << running->pc << endl;
		cout << "BASE VM -> PCB: " << vm.base << " -> " << running->base << endl;
		cout << "LIMIT VM -> PCB: " << vm.limit << " -> " << running->limit << endl;
		cout << "STACK POINTER VM -> PCB: " << vm.stackPointer << " -> " << running->limit; 
		cout << endl;
		cout << "LOAD/STORE: " << running->loadWordStoreWord << endl;
		cout << "---------------------------------" << endl;
	}
	
	/////////// STACK ////////////
	//check size of stack for largest stack size
	if((MEM_SIZE - vm.stackPointer) > running->largest_stack)
	{	
			running->largest_stack = (MEM_SIZE - vm.stackPointer); //largest stack size = stack 
	}
	running->stackPointer = vm.stackPointer;
	running->statusRegister = vm.statusRegister;
	running->regs = vm.regs;
	
    if(vm.stackPointer < MEM_SIZE) //there is a stack
    {
		//open stack file
		string stackfile = running->process + ".st";
		running->st.open(stackfile.c_str(), ios::out);
		//i is the very bottom
		int i = MEM_SIZE;
		if(DEBUG > 0)
		{
			cout << "Writing to Stack File for Process: " << running->process << endl;
		}
		while(i > vm.stackPointer)
		{
			//starting from the bottom, write whats inside mem into stack file
			running->st << vm.mem[--i] << endl;
			if(DEBUG > 0)
			{
				cout << i << ": " << vm.mem[i] << endl;
			}
		}	
		running->st.close();
   	}
	return;
}

/*///////////////////////////////////////////////////////////////////////////////////////////////
*
* Function: read
*
* Description: handles input instructions and places them into the correct register.
*			the input data will be in the "in" stream of the PCB, which is a file
*			containing the data needed. Each data item is separated by a new line.
*
* Inputs: int reg - register that the data will be placed in
*
* Outputs: none
*
/*///////////////////////////////////////////////////////////////////////////////////////////////
void OS::read(int reg)
{
	//read from write file and place it inside regs[reg] of PCB
	running->in >> running->regs[reg];
	return;
}

/*///////////////////////////////////////////////////////////////////////////////////////////////
*
* Function: write
*
* Description: handles output instructions and places them into the correct register.
*			Writes the data in the specified register into the "out" stream of the
*			PCB, which is a file containing all outputs for the program.
*
* Inputs: int reg - register that contains data to write out to file.
*
* Outputs: none
*
/*///////////////////////////////////////////////////////////////////////////////////////////////
void OS::write(int reg)
{
	//open readfile
	//write regs[reg] inside PCB
	if(running->regs[reg] > 32767) //negative values
	{   
		int temp;
	   	temp = running->regs[reg] | 4294901760; //sign extend to 32 bits
	    running->out << "Output: " << temp << endl;
	}	
	else //positive values
	{
		running->out << "Output: " << running->regs[reg] << endl;
	}
	return;
}

/*///////////////////////////////////////////////////////////////////////////////////////////////
*
* Function: checkwaitQ
*
* Description: checks the wait queue to see if any processes have waited long enough
*			to simulate a hardware IO request. If a program has, we will move it to
*			the ready queue. 
*
* Inputs: none
*
* Outputs: none
*
/*///////////////////////////////////////////////////////////////////////////////////////////////
void OS::checkwaitQ()
{	
	if(!waitQ.empty())
	{
		//if I/O complete
		if((vm.clock+5) >= (waitQ.front()->wait_end))
		{
			//move to readyQ
			readyQ.push(waitQ.front());
			//pop waitQ
			waitQ.pop();
			
			//stop io time. add the difference of clock and original io_time
			readyQ.back()->io_time += (vm.clock - readyQ.back()->io_start); 
			//start wait time		
			readyQ.back()->ready_start = vm.clock; 
		}
	}
	return;
}
/*///////////////////////////////////////////////////////////////////////////////////////////////
*	function: get_victim
*
*	description: this will get the next frame to kick out of memory so that a new page can be 
*				loaded. First, look for a free frame in memory by checking the 
*				inverted_page_table valid bit. If not, look for a victim. We use two separate 
*				algorithms to find a victim:
*				
*				FIFO - (First In First Out) - uses a queue to determine which frame to kick out. 
*							As soon as a page is loaded into memory, that number is added to 
*							a queue of integers. When looking for a victim, uses the front of 
*							the queue. 
*
*				LRU - (Least Recently Used) - uses a register for each frame which resides in
*							the VM. Timestamps the register as soon as a page is loaded 
*							into the frame. When looking for a victim, uses the frame that 
*							has the lowest value register.
*		
*	inputs: none
*
*	outputs: int victim - frame number to kick out of memory and load a new page into
*
*									Inverted Page Table 
* 	-------------------------------------------------------------------------------------------
*	| 15 | 14 | 13 | 12 | 11 | 10 |  9  |  8  |  7  |  6  |  5  |  4  |  3  |  2  |  1  |  0  |
*	-------------------------------------------------------------------------------------------
* 	|  PROCESS ID   		 | 	   (Page)     1024 Maximum Pages for a Program  	    |VALID|
*	-------------------------------------------------------------------------------------------
*
/*///////////////////////////////////////////////////////////////////////////////////////////////
int OS::get_victim()
{
	int victim = -1;
	int free_frame = 1;
	int j = 0;
	while(free_frame != 0 && j < inverted_page_table.size())//while frame is not free
	{
		free_frame = (inverted_page_table[j] & 1); //extract valid/invalid bit
		if(free_frame == 0)
		{	
			if(alg == "fifo"){
				fifo.push(j);
			}
			return j;
		}
		j++;
	}
	   ///// NO FREE FRAMES /////
	///// MUST REPLACE A PAGE /////
	if(alg == "lru") //LRU
	{
		int x = vm.lruFrame[0];
		int i = 0;
		for(i = 1; i < vm.lruFrame.size(); i++) //go through all frames & get the smallest time
		{
			if(vm.lruFrame[i] < x)
			{
				x = vm.lruFrame[i];
				victim = i;
			}		
		}
		if(victim == -1)
		{
			victim++;
		}
		if(DEBUG > 0)
		{
			cout << "Victim: " << victim << endl;
		}
		reset_page_table_entry(victim);	 
		return victim;

	}
	else if(alg == "fifo") //FIFO
	{	
		victim = fifo.front();  //pick victim from queue
		fifo.pop();
		reset_page_table_entry(victim);
		fifo.push(victim); //push vitcim back on the end of the queue
		return victim;
	}
}

/*///////////////////////////////////////////////////////////////////////////////////////////////
*	function: set_inverted
*
*	description: the inverted_page_table is a vector of integers. Each entry represents the 
*			corresponding frame in memory. We should not need to clear because as another page
*			from another process gets loaded into memory, it will overwrite the previous entry
*
*	inputs: int frame - frame in memory 
*			int p_id - Process ID
*			int page - Page loaded in memory
*
*	outputs: none
*															ENTRY
*	-------------------------------------------------------------------------------------------
* 	| 15 | 14 | 13 | 12 | 11 | 10 |  9  |  8  |  7  |  6  |  5  |  4  |  3  |  2  |  1  |  0  |
*	-------------------------------------------------------------------------------------------
* 	|  PROCESS ID 			 | 	   (Page)     1024 Maximum Pages for a Program  	    |VALID|
*	-------------------------------------------------------------------------------------------
*
/*///////////////////////////////////////////////////////////////////////////////////////////////
void OS::set_inverted(int frame, int p_id, int page)
{
	int entry = p_id << (MAX_PAGE_BITS + 1); //set process_id
	entry += (page << 1); //set page number
	entry |= 1; //set valid bit
	inverted_page_table[frame] = entry;
	
	if(DEBUG > 0){
		cout << "SET INVERTED CALLED" << endl; 
		cout << "-----------------Inverted Page Table--------------" << endl;
		for(int i = 0; i < inverted_page_table.size(); i++)
		{ 
			if(i != 0 &&  (i % 8) == 0){
				cout << endl;
			}
			int p = (inverted_page_table[i] >> (MAX_PAGE_BITS + 1));
			int pa = ((inverted_page_table[i] >> 1) & 1023);
			cout << "Frame";
			if(i < 10){
				cout << "0";
			}
			cout << i << "=" << p << "-" << pa << "||";
			
		}
		cout << endl;
	}
}

/*///////////////////////////////////////////////////////////////////////////////////////////////
*	function: reset_page_table_entry
*
*	description: resets the page entry corresponding to the page that was kicked out of memory.	
*				We will go through the inverted page table to find the process and page number 
*				and then go into the pagetable for the process we extract out of the inverted 
*				page table
*				
*	inputs: int frame - frame in memory 
*
*	outputs: none
*
/*///////////////////////////////////////////////////////////////////////////////////////////////
void OS::reset_page_table_entry(int frame)
{
	//extract page and process  out of inverted page table
	int p_id = inverted_page_table[frame] >> (MAX_PAGE_BITS + 1);
	int page = inverted_page_table[frame] >> 1;
	page &= (MAX_PAGE); 

	list<PCB*>::iterator it = jobs.begin();
    for(it; it != jobs.end(); it++)
	{
	   	if((*it)->process_id == p_id)
		{
			//check if the page we are kicking out has been modified. 
			if((*it)->pagetable.get_mod(page)) 
			{
				if(DEBUG >0){
					cout << "MODIFY PAGE:" << page << " for PROCESS: " << (*it)->process;
					cout << endl;
				}
				//if so, modify the page we are kicking out, so we do not lose data
				modify(page);
			}
			(*it)->pagetable.table[page] = 0; //reset the page in the page table.
			if(DEBUG > 0)
			{
				cout << "*****PERFORM RESET PAGE TABLE*****" << endl;
				cout << "Process: " << (*it)->process << ". Page: " << page << "." << endl;
				cout << "Inverted Page Table Entry at frame " << frame << ": ";
				cout << inverted_page_table[frame] << endl;
				cout << endl;
			}
			return;
		}
	}
}

/*///////////////////////////////////////////////////////////////////////////////////////////////
*	function: get_logical
*
*	description: gets the logical address in the program. We will first check to see if the page 
*				is loaded into memory. If so, get the base address of the page we are in then add 
*				the offset of the address in RAM. (PhysicalAddress % PAGE_SIZE).
*				
*	inputs: int address_in_memory - address in physical memory
*
*	outputs: logical_address - logical address of the pc relative to the program
*									
/*///////////////////////////////////////////////////////////////////////////////////////////////
int OS::get_logical(int address_in_memory)
{	
	int logical_address = 0;
	int offset = (address_in_memory % PAGE_SIZE);
	int page_base_address = 0;
	int page = 0;
	if((inverted_page_table[address_in_memory/PAGE_SIZE] & 1) == 1) //if the page is in memory
	{ 
		//extract page number
		page = (inverted_page_table[address_in_memory/PAGE_SIZE]) & (MAX_PAGE);
		page >>= 1;
		
		page_base_address = (page * PAGE_SIZE); //starting address of that page
		logical_address = page_base_address + offset; //logical address in program
		
		return logical_address;
	}
	else
	{
		cout << "Invalid logical reference on page not loaded in memory" << endl;
		return -1;
	}
}
/*///////////////////////////////////////////////////////////////////////////////////////////////
* Function: modify
*
*
* Description: Modifies the program located in the hard disk if any data was stored inside
*			the programs limits. (Typically noop instructions). We will store all the lines
*			of the current object file in a vector. To get the new page, we will continue to
*			get those pages but ignore the content. When we are loading that page, we will
*			take the objects in memory that we have modified. Once we are finished with the
*			page, we will continue on to the rest of the file. Once we have all of the objects
*			we will then erase the current file and write all of the vector elements into the
*			new object file. We do this because the amount of characters in a file cannot 
*			decrease once it is created. It can only increase the amount of characters. If 
*			the modified line is smaller than the current instruction, we will need to decrease
*			the amount of characters in that line. This method is a work around.
*
* Inputs: page - page number to write back to hard disk
*
* Outputs: none
*	
/*///////////////////////////////////////////////////////////////////////////////////////////////
void OS::modify(int page)
{
	string oFile = running->process + ".o";
	vector<int> obj;
	string line;
	int intline;
	
	running->o.clear();
	running->o.seekg(ios::beg); //move get pointer to beginning of file
	//go to the desired page number by ignoring the rest of the lines of code
	for(int i = 0; i < (page * PAGE_SIZE); i++)
	{
		getline(running->o, line);
		istringstream str(line.c_str());
		str >> intline;
		obj.push_back(intline);
	}
	
	//get the contents for this page out of memory
	int instr = running->pagetable.get_frame(page) * PAGE_SIZE;
	for(int i = 0; i < PAGE_SIZE; i++)
	{
		//get the line in memory
		getline(running->o, line);
		obj.push_back(vm.mem[instr]);
		instr++;
		
		//-1 in memory means free space and not used in the program, if -1, don't
		// get a new line because that line does not exist in the program's object file.
		// set i to page_size and stop getting the page
		if(vm.mem[instr] == -1){ 
			i = PAGE_SIZE;
		}
	}
	if(!running->o.eof()){
		
		//get the rest of the pages out of the object file
		getline(running->o, line);
		while(!running->o.eof())
		{
			istringstream str(line.c_str());
			str >> intline;
			obj.push_back(intline);
			getline(running->o, line);
		}
	}
	// close the stream and reopen it with the trunc flag, to erase all previous content.
	running->o.close();
	running->o.open(oFile.c_str(),  ios::out | ios::trunc);
	
	//write the contents of the vector we created onto the new objectfile
	for(int i = 0; i < obj.size(); i++)
    { 
	    running->o << obj[i] << endl;
		if(DEBUG > 0)
		{
			if(i == 0){
					cout << "Modifying Process " << running->process << ", Page: " << endl;
			}
			cout << "OBJ " << i+1 << ": " << obj[i] << endl;
		}
    }
	
	running->pagetable.set_modify_bit(page, 0); //reset modify bit;
	
	//close file and reopen with different flags in and out
	running->o.close();
	running->o.open(oFile.c_str(),  ios::in | ios::out);
	obj.clear(); //clears content of vector to prevent overlap of file instructions
}
/*///////////////////////////////////////////////MAIN////////////////////////////////////////////
* Function: main
*
*
* Description: prints out system information calculated during the execution of all jobs and 
*			context switches. This will write to all jobs' output files.
*
* Inputs: none
*	
*
* Outputs: none
*
/*///////////////////////////////////////////////////////////////////////////////////////////////

void OS::system_info(int level)
{
	if(level == SYS_INFO_PROCESS_HALT)
	{
		running->out << "------Accounting Information------" << endl;
		running->out << "CPU: " << running->cpu_time << endl;
		running->out << "Waiting Time: " << running->wait_time << endl;
		running->out << "Turnaround: " << running->turnaround_time << endl;
		running->out << "I/O Time: " << running->io_time << endl;
		running->out << "Largest Stack Size: " <<  running->largest_stack << endl;
		running->out << "Page Faults: " << running->page_fault_count << endl;
		running->out << "Stack Faults: " << running->stack_fault_count << endl;
		running->out << endl;
	}
	else if (level == SYS_INFO_OS_HALT)
	{
		list<PCB*>::iterator it = jobs.begin();
		for(it; it != jobs.end(); it++) 
		{
			user_cpu += (*it)->cpu_time;
			hitratio += (*it)->page_fault_count;
		}
		hitratio += vm.hit;
		hitratio = vm.hit/hitratio * 100;
		user_cpu = (user_cpu / (double)vm.clock) * 100;
		system_time = context_time + idle_time; //time of all context switches
		system_cpu = ( ((double)vm.clock - (double)idle_time) / (double)vm.clock ) * 100.00;

		throughput = (double)jobs.size() / ((double)vm.clock/(double)1000);
	
		it = jobs.end();
		it--;
	
		if(DEBUG > 0)
		{
			cout << "Finished All Processes. Process Count: " << jobs.size() << endl;
			cout << (*it)->process << endl;
		}
		for(it = jobs.begin(); it != jobs.end(); it++) 
		{
			if(DEBUG > 0)
			{
				cout << (*it)->process << endl;
			}
			(*it)->out << "System Time: " << system_time << endl;
			(*it)->out << "System CPU Utilization:" << system_cpu << endl;
			(*it)->out << "User CPU Utilization: " << user_cpu << endl;
			(*it)->out << "Throughput: " << throughput << endl;	
			(*it)->out << "Hit Ratio: " << hitratio << endl;	
			(*it)->out.close();
			(*it)->in.close();
		}
	}
	return;
}
/*///////////////////////////////////////////////MAIN////////////////////////////////////////////
* Function: main
*
*
* Description: instantiates an OS and then calls OS.run()
*
* Inputs: argv[1] - page replacement algorithm used. Either fifo or lru
*	
*
* Outputs:  During the run, multiple .out files should be created that represents output for each
*			of the programs running during the life of the OS.
*
/*///////////////////////////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[])
{
    //Keeping use of args by casting method
    char* progList = argv[1];
	char* page_replacement_algorithm = argv[2];
	
	if(DEBUG > 0)
	{
		cout << "STARTING OS WITH PROGRAM FILE: " << progList << endl;
		cout << "Page Replacement Algorithm: " << page_replacement_algorithm << endl; 
	}
	
	//Instantiate OS
    OS os(progList, page_replacement_algorithm);

    //run os
    os.run();

    return 0;
}//main
