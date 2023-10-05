
/*******************************************************************************************
 *
 * File: VirtualMachine.cpp
 *
 * Name(s): Jacob LeReaux
 *
 * Last Modified: March 28, 2018
 *
 * Description: This code is an implementation of a 16-bit Virtual Machine (VM).
 *    The Virtual Machine will take a PCB and execute instructions accordingly. The VM will 
 *   exit when the timeslice is up, or if there are any routines in which the OS needs to 
 *   complete. The OS will handle all Read and Write routines. The OS will also gain control
 *   if a Page Fault occurs. In the VM, we have a Translation Look-Aside Buffer which will
 *  be a copy of the current running program's Page Table. The VM has a number of general
 *    purpose registers, status register, base and limit registers, as well as a program
 *   counter, which will all be copied over from the running program's PCB before entering
 *   the VM. When loading instructions into memory, the VM will load -1 instruction into 
 *   any free addresses. For page replacement, we have a FIFO queue and LRU registers, in 
 *   which the LRU registers are stored in the VM. We will update those registers whenever
 *   a specific frame is used. Lastly, each program has their own stack. For a specific
 *  program, it only will write onto the bottom-most addresses and move its way backwards. 
 *  Any program that has been modified, mainly for a store operation, we will update the 
 *    program's page table accordingly, so that the OS knows to write that frame to the hard 
 *   disk.
 *
 *******************************************************************************************/
 
 #include "VirtualMachine.h"
 
 
VirtualMachine::VirtualMachine()
{
   regs = vector<int>(NUMBER_OF_REGISTERS);
   mem = vector<int>(MEM_SIZE);
   lruFrame = vector<int>((MEM_SIZE/FRAME_SIZE),0);
   pc = 0;
   statusRegister = 0;
   stackPointer = MEM_SIZE;
   limit = 0; 
   base = 0;
   clock = 0;
   instr.op = 0;
   instr.destReg = 0;
   instr.immediate = 0;
   instr.sourceReg = 0;
   instr.constant = 0;
   instr.addr = 0;
   hit = 0 ;//number of times TLB was successful
}

/*///////////////////////////////////////////////////////////////////////////////////////////////
* FUNCTION: load_page()
*
* DESCRIPTION: goes into the object file for a program and takes the specified page
*   of instructions and places them into memory. If any space is left over (if it is 
*   the last page of a program), we will load noop instructions into the remaining 
*   spaces in the corresponding frame number. This function also sets the base and 
*   limit values into the program's PCB, as well as the PageTable
*
* Inputs: fstream &objectfile   - file that contains the instructions for the program
*         PCB* p              - Program control block
*         int page          - page number of the program to load into  memory
*         int frame         - frame to load the page into
*
* Outputs: none
*
/*///////////////////////////////////////////////////////////////////////////////////////////////

void VirtualMachine::load_page(fstream &objectfile, PCB* p, int page, int frame)
{
   int virtual_addr = 0;
   if (!objectfile.is_open())
   {

      cout << "prog.s failed to open.\n";
      exit(1);
   }
   int k = 0;
   objectfile.clear();
   objectfile.seekg(ios::beg); //move get pointer to beginning of file
   
   //go to the desired page number by ignoring the rest of the lines of code
   for(int i = 0; i < (page * FRAME_SIZE); i++) 
   {
      string ignore;
      getline(objectfile, ignore);
      k++;
   } 
   virtual_addr = frame * FRAME_SIZE;
   limit = 0; //reset limit 
   while(!objectfile.eof() && limit < PAGE_SIZE && virtual_addr < MEM_SIZE) 
   {  
   objectfile >> mem[virtual_addr];
   limit++;
   virtual_addr++;
   }
   if (limit != PAGE_SIZE)
   {
      //if the frame has leftover space, fill rest of frame to noops
      for(int i = 0; i < (FRAME_SIZE - limit) + 1; i++) 
      {
         mem[virtual_addr] = -1; //load -1 into remaining spaces
         virtual_addr++;
      }
   }
   
   if(DEBUG > 0)
   {
      cout << p->process <<  " LOADING PAGE: " << page << " INTO FRAME " << frame << endl;
      cout << "-------MEM------" << endl;   
      for(int i = frame*FRAME_SIZE; i < (frame+1)*FRAME_SIZE; i++){
         cout << i << ": " << mem[i] << endl;  
      }
   }
   lruFrame[frame] = clock; //time stamp
   p->pagetable.set_frame(page, frame); //setting page table to point to current frame
}

/*///////////////////////////////////////////////////////////////////////////////////////////////
*   FUNCTION: run()
*
*   DESCRIPTION: Executes current program indicated through inputed Program Control Block (PCB). 
*      Uses bit extraction to gather OP_CODE, Registers (Source and Destination), IMMEDIATE 
*      VALUE, to perform instruction inside IF ELSE IF BLOCK. Executes until timeslice is 
*      reached, IO has been requested, or a halt instruction. Will then return to OS. OS handles 
*      all context switching and IO requests and places PCB in appropriate queue (Wait or Ready).
*      Contains status register that is updated when status changes, which OS has access to.
*
*
*                           INSTRUCTION REGISTER
*   -------------------------------------------------------------------------------------------
*   | 15 | 14 | 13 | 12 | 11 | 10 |  9  |  8  |  7  |  6  |  5  |  4  |  3  |  2  |  1  |  0  |
*   -------------------------------------------------------------------------------------------
*   |       OP_CODE         | Dest_Reg |  i  |Source_Regi|            DONT CARE              |
*   -------------------------------------------------------------------------------------------
*
*                     INSTRUCTION REGISTER WITH IMMEDIATE
*   -------------------------------------------------------------------------------------------
*   | 15 | 14 | 13 | 12 | 11 | 10 |  9  |  8  |  7  |  6  |  5  |  4  |  3  |  2  |  1  |  0  |
*   -------------------------------------------------------------------------------------------
*   |       OP_CODE         | Dest_Reg |  i  |   IMMEDIATE VALUE  (CONSTANT OR ADDRESS)     |
*   -------------------------------------------------------------------------------------------
*
*
*
*                            STATUS REGISTER
*    -------------------------------------------------------------------------------------------
*   | 15 | 14 | 13 | 12 | 11 | 10 |  9  |  8  |  7  |  6  |  5  |  4  |  3  |  2  |  1  |  0  |
*   -------------------------------------------------------------------------------------------
*    |            DONT CARE   | PF |  i/O Reg  | VM ReturnStatus |  V  |  L  |  E  |  G  |  C  |
*   -------------------------------------------------------------------------------------------
*   PF - Page Fault. V - Overflow. L -Less Than. E - Equal To. G - Greater Than. C - Carry Bit 
*
/*///////////////////////////////////////////////////////////////////////////////////////////////

void VirtualMachine::run(PCB* running)
{
   int pid = running->process_id;
   //go through TLB to find page. If page is not loaded into memory - page fault
   if(pc >= (limit))
   {
      pc = convertAddressTLB(running->pc);
      if(pc < 0){
         setStatus(page_fault);
         return;
      }
   }
   int incr;
   clock_start = clock;
      int slice = clock + 15;   //alloted time for timeslice (15 clock ticks)
   int logicalAddr = 0; //Used for calls and returns
   int physicalAddress = -1;

    ////// fetch-execute cycle /////////
    while(clock < slice)
    {
      lruFrame[pc/FRAME_SIZE] = clock;
      int instr_reg = mem[pc]; //fetch instruction from memory
      if(DEBUG > 1){
         cout << running->process << " PC: " << running->pc << endl;
         cout << "INSTR = " << instr_reg << endl;
         cout << "PC: " << pc << endl;
         cout << "R0: " << regs[0] << endl;
         cout << "R1: " << regs[1] << endl;
         cout << "R2: " << regs[2] << endl;
         cout << "R3: " << regs[3] << endl;
         cout << "SP: " << stackPointer << endl;
         cout << "-------------------------------" << endl;
      }
      incr++;
      pc++;  //move to next location
      //Bit Extraction
       instr.op         = (instr_reg & op_mask)    >> 11; //set opcode
      instr.destReg    = (instr_reg & rd_mask)    >> 9; //set destination registers
       instr.immediate  = (instr_reg & i_mask)     >> 8; //set immediate
       instr.sourceReg  = (instr_reg & rs_mask)    >> 6; //set source register
      instr.addr       = (instr_reg & addr_mask)  >> 0; //set address   
       instr.constant   = (instr_reg & const_mask) >> 0; //set constant
      if( instr.constant > 127) // if constant is greater than 127, sign bit will be high
      {
      //sign extend to 16 bits, since the actual machine this is running on is 32 bits. 
         instr.constant |= 65280; 
      }
      
      /////////////////////////////////////////////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////////////////////
      ///////////////////////////////  OP_CODE ROUTINES  //////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////////////////////

      if(instr.op == 0 && instr.immediate == 0)//load
      {
         if((slice - clock) < 4) // if not enough time to complete instruction
         {
            slice = clock + 4; // extend timeslice
         }      
         // input the logical address and return physical address through TLB lookup
         physicalAddress = convertAddressTLB(instr.addr); 
         
         if(physicalAddress < 0) // Address does not exist in memory
         {
            //load or store word address location. This will equal zero prior to this
            //the os will check this value. If it is greater than zero, then we do need to
            //load a page from memory so that we can get to the address needed to complete
            //the operation. 
            running->loadWordStoreWord = instr.addr; 
            pc--; // PC - 1 We have not completed the instruction yet.
            setStatus(page_fault);
            return;
         }
         
         regs[instr.destReg] = mem[physicalAddress];
         clock += 4;
      }
      else if(instr.op == 0 && instr.immediate == 1)//loadi
      {
         regs[instr.destReg] = instr.constant;
         clock++;
      }
      else if(instr.op == 1) //store
      {
         // if not enough time to complete instruction
         if((slice - clock) < 4)
         {
            slice = clock + 4;
         }      
         
         // input the logical address and return physical address through TLB lookup
         physicalAddress = convertAddressTLB(instr.addr); 
         
         if(physicalAddress < 0) // Address does not exist in memory
         {
            //load or store word address location. This will equal zero prior to this
            //the os will check this value. If it is greater than zero, then we do need 
            //to load a page from memory so that we can get to the address needed to 
            //complete the operation. 
            running->loadWordStoreWord = instr.addr; 
            pc--; // PC - 1 We have not completed the instruction yet.
            setStatus(page_fault);
            return;
         }
         mem[physicalAddress] = regs[instr.destReg]; //store operation
         
         //set modify bit so we know to update the page in the object file
         TLB.set_modify_bit((instr.addr/PAGE_SIZE), 1); 
         
         clock += 4; 
      }
      else if(instr.op == 2 && instr.immediate == 0)//add
      {
         int sum = regs[instr.destReg] + regs[instr.sourceReg];
         //set overflow by checking both numbers and their sum
         setoverflow(regs[instr.destReg],regs[instr.sourceReg],sum); 
         regs[instr.destReg] += regs[instr.sourceReg];
         setcarry();
         clock++;
      }
      else if(instr.op == 2 && instr.immediate == 1)//addi
      {
         int sum = regs[instr.destReg] + instr.constant;
         //set overflow by checking both numbers and their sum
         setoverflow(regs[instr.destReg],instr.constant,sum); 
         regs[instr.destReg] += instr.constant;
         setcarry(); 
         clock++;
      }
      else if(instr.op == 3 && instr.immediate == 0)//addc
      {
         int sum = regs[instr.destReg] + regs[instr.sourceReg] + carry();
         //set overflow by checking both numbers and their sum
         setoverflow(regs[instr.destReg],regs[instr.sourceReg] + carry(),sum);
         regs[instr.destReg] += regs[instr.sourceReg] + carry();
         setcarry();
         clock++;
      }
      else if(instr.op == 3 && instr.immediate == 1)//addci
      {
         int sum = regs[instr.destReg] + instr.constant + carry();
         //set overflow by checking both numbers and their sum
         setoverflow(regs[instr.destReg],instr.constant + carry(),sum);
         regs[instr.destReg] += instr.constant + carry();
         setcarry();
         clock++;
      } 
      else if(instr.op == 4 && instr.immediate == 0)//sub
      {  
         regs[instr.destReg] -= regs[instr.sourceReg];
         setcarry();
         clock++;
      }
      else if(instr.op == 4 && instr.immediate == 1)//subi
      {
         regs[instr.destReg] -= instr.constant;
         setcarry();
         clock++;
      }
      else if(instr.op == 5 && instr.immediate == 0)//subc
      {
         regs[instr.destReg] -= regs[instr.sourceReg] - carry();
         setcarry();
         clock++;   
      }
      else if(instr.op == 5 && instr.immediate == 1)//subci
      {
         regs[instr.destReg] = regs[instr.destReg] - instr.constant - carry();
         setcarry();
         clock++;
      }
      else if(instr.op == 6 && instr.immediate == 0)//and
      {
         regs[instr.destReg] &= regs[instr.sourceReg];
         clock++;
      }
      else if(instr.op == 6 && instr.immediate == 1)//andi
      {
         regs[instr.destReg] &= instr.constant;
         clock++;
      }
      else if(instr.op == 7 && instr.immediate == 0)//xor
      {
         instr.destReg ^= regs[instr.sourceReg];
         clock++;
      }
      else if(instr.op == 7 && instr.immediate == 1)//xori
      {
         instr.destReg ^= instr.constant;
         clock++;
      }
      else if(instr.op == 8)//compl
      {
         instr.destReg ^= 65553; //xor with 111111111111111 to get compl
         clock++;
      }
      else if(instr.op == 9)//shl
      {
         setcarry();
         regs[instr.destReg] = regs[instr.destReg] << 1;
         clock++;
            
         if(regs[instr.destReg] > 65535)
         {
            setcarry();
         }
      }
      else if(instr.op == 10)//shla
      {
         setcarry();
         if(regs[instr.destReg] > 32767) 
         {
            regs[instr.destReg] = regs[instr.destReg] << 1;   
            regs[instr.destReg] |= 32768;   
         }
         else
         {   
            regs[instr.destReg] = regs[instr.destReg] << 1;   
         }
           
         clock++;
      }
      else if(instr.op == 11)//shr
      {
         if((regs[instr.destReg] & 1) == 1)
         {
            statusRegister |= 1;
         }
         else
         {
            statusRegister &= 30;
         }
         regs[instr.destReg] = regs[instr.destReg] >> 1;
         clock++;
      }
      else if(instr.op == 12)//shra
      {
         if((regs[instr.destReg] & 1) == 1) 
         {
            statusRegister |= 1;
         }
         else
         {
            statusRegister &= 30;
         }

         if(regs[instr.destReg] > 32767)
         {
            regs[instr.destReg] = regs[instr.destReg] >> 1;   
            regs[instr.destReg] |= 32768;   
         }
         else
         {   
            regs[instr.destReg] = regs[instr.destReg] >> 1;   
         }

         clock++;
      }
      else if(instr.op == 13 && instr.immediate == 0)//compr
      {
         if(regs[instr.destReg] < regs[instr.sourceReg])
         {
            setless();
         }
         else if(regs[instr.destReg] == regs[instr.sourceReg])
         {
               setequal();
         }
         else if(regs[instr.destReg] > regs[instr.sourceReg])
         {
            setgreater();
         }
           
         clock++;
      }
      else if(instr.op == 13 && instr.immediate == 1)//compri
      {
         if(regs[instr.destReg] < instr.constant)
         {
               setless();
         }
         else if(regs[instr.destReg] == instr.constant)
         {
            setequal();
         }
         else if(regs[instr.destReg] > instr.constant)
         {
               setgreater();
         } 
         clock++;
      }
      else if(instr.op == 14)//getstat
      {
         regs[instr.destReg] = statusRegister;
         clock++;
      }
      else if(instr.op == 15)//putstat
      {
         statusRegister = regs[instr.destReg];
         statusRegister &= 31;
         clock++;
      }
      else if(instr.op == 16 && instr.immediate == 1)//jump
      {
         // input the logical address and return physical address through TLB lookup
         physicalAddress = convertAddressTLB(instr.addr); 
         
      if(physicalAddress < 0)
      {
         // Address does not exist in memory
         pc = instr.addr; //set the pc to the logical address before returning to OS
         setStatus(page_fault);
         return;
      }
         //Page is loaded into memory, so readjust base and limit registers accordingly
         base = (physicalAddress / FRAME_SIZE) * FRAME_SIZE;
         running->base = (instr.addr / PAGE_SIZE) * PAGE_SIZE;
         limit = base + PAGE_SIZE;
         //set pc to physical address
         pc = physicalAddress;
         clock++;
      }
      else if(instr.op == 17 && instr.immediate == 1)//jumpl
      {
         if(is_less() == 1)
         {
            // input the logical address and return physical address through TLB lookup
            physicalAddress = convertAddressTLB(instr.addr); 
            
            if(physicalAddress < 0)
            { 
               // Address does not exist in memory
               pc = instr.addr; //set the pc to the logical address before returning to OS
               setStatus(page_fault);
               return;
            }
            //Page is loaded into memory, so readjust base and limit registers accordingly
            base = (physicalAddress / FRAME_SIZE) * FRAME_SIZE;
            running->base = (instr.addr / PAGE_SIZE) * PAGE_SIZE;
            limit = base + PAGE_SIZE;
            //set pc to physical address
            pc = physicalAddress;
         }
         clock++;
      }
      else if(instr.op == 18 && instr.immediate == 1)//jumpe
      {
         if(is_equal() == 1)
         {  
            // input the logical address and return physical address through TLB lookup
            physicalAddress = convertAddressTLB(instr.addr); 
            
            if(physicalAddress < 0) 
            { 
               // Address does not exist in memory
               pc = instr.addr; //set the pc to the logical address before returning to OS
               setStatus(page_fault);
               return;
            }
            //Page is loaded into memory, so readjust base and limit registers accordingly
            base = (physicalAddress / FRAME_SIZE) * FRAME_SIZE;
            running->base = (instr.addr / PAGE_SIZE) * PAGE_SIZE;
            limit = base + PAGE_SIZE;
            //set pc to physical address
            pc = physicalAddress;
         }
         clock++;
      }
      else if(instr.op == 19 && instr.immediate == 1)//jumpg
      {
         if(is_greater() == 1)
         {
            // input the logical address and return physical address through TLB lookup
            physicalAddress = convertAddressTLB(instr.addr); 
            
            if(physicalAddress < 0)
            { 
               // Address does not exist in memory
               pc = instr.addr; //set the pc to the logical address
               setStatus(page_fault);
               return;
            }
            //Page is loaded into memory, so readjust base and limit registers accordingly
            base = (physicalAddress / FRAME_SIZE) * FRAME_SIZE;
            running->base = (instr.addr / PAGE_SIZE) * PAGE_SIZE;
            limit = base + PAGE_SIZE;
            //set pc to physical address
            pc = physicalAddress;
         }
         clock++;
      }
      else if(instr.op == 20 && instr.immediate == 1)//call
      {
         if(DEBUG > 0)
         {
            cout << "CALL for Process: " << running->process << endl;
            cout << "Needed Stack Pointer: " << (stackPointer - 6) << endl;
         }
         //--------------------- TIME ----------------------//
         if((slice - clock) < 4) // if not enough time to complete instruction
         {
            slice = clock + 4;
         }         
         //--------------------------------------------------------------------------------
         //-------- CHECK IF OUR PC IS IN A FRAME THAT NEEDS TO BE KICKED OUT --------//
         //Top most frame we will need for stack after a call.
         int stackFrameNeeded = (stackPointer - 6) / FRAME_SIZE;
         
         int pcFrame = pc / FRAME_SIZE;
         if(pcFrame >= stackFrameNeeded)
         {
            if(DEBUG > 0)
            {
               cout << "PC is inside of needed frame for stack" << endl;
            }
            //if the frames we are going to use contains our PC
            pc--; // PC - 1 We have not completed the instruction yet.
            setStatus(stack_overflow + page_fault);
            return;
         }
         //--------------------------------------------------------------------------------
         
         //-------- CHECK IF A PAGE THAT HAS BEEN MODIFIED NEEDS TO BE KICKED OUT --------//
         //Overwrite any memory inside of stack frames. We will kick them out of memory when 
         // we return to the OS. If the frames needed for stack belong to our program and 
         // they have been modified, we must return to OS first to write our modifications 
         // to hard memory.
         for(int i = 0; i < TLB.table.size(); i++)
         {   //if the page is in memory
            if(TLB.get_valid(i))
            {   //if we are writing stack inside one of the frames 
               // that belongs to our current program
               if(TLB.get_frame(i) >= stackFrameNeeded)
               {   //if we modified it during this execution
                  if(TLB.get_mod(i))
                  {
                     if(DEBUG > 0)
                     {
                        cout << "One of the frames needed has been modified" << endl;
                        cout << "       - Page: " << i << endl;
                     }
                     //we need to return because we are about to overwrite one
                     // of our pages that has been modified during execution
                     pc--; // PC - 1 We have not completed the instruction yet.
                     setStatus(stack_overflow + page_fault);
                     exit(0);
                     return;
                  }
               }
            }
         }
         //--------------------------------------------------------------------------------

         //-------------------------- COMPLETE INSTRUCTION --------------------------//
         //if there are at least 6 open memory spaces between top of stack and limit;
         if(stackPointer > (MEM_SIZE / 2) )
         {
            logicalAddr = running->base + (pc % PAGE_SIZE);
            mem[--stackPointer] = logicalAddr; //push program counter
                
            for(int i = 0; i < 4; i++)
            {
               mem[--stackPointer] = regs[i]; //push all 4 registers
            }
                
            mem[--stackPointer] = statusRegister; //push status register
            clock += 4;
         }
         else
         { 
            clock += 4;
            setStatus(stack_overflow); //set status register appropriately
            return;
         }   
         
         //------------------------------ PERFORM JUMP -------------------------------//
         // input the logical address and return physical address through TLB lookup
         physicalAddress = convertAddressTLB(instr.addr); 
         
         if(physicalAddress < 0)
         { 
            if(DEBUG > 0)
            {
               cout << "CALL RETURNING TO OS DUE TO JUMP ADDRESS PAGE FAULT" << endl;
               cout << endl;
            }
            // Address does not exist in memory
            pc = instr.addr; // PC - 1. We have not completed the instruction yet.
            setStatus(page_fault);
            return;
         }
         //Page is loaded into memory, so readjust base and limit registers accordingly
         base = (physicalAddress / FRAME_SIZE) * FRAME_SIZE;
         running->base = (instr.addr / PAGE_SIZE) * PAGE_SIZE;
         limit = base + PAGE_SIZE;
         //set pc to physical address
         pc = physicalAddress;
         //----------------------------------------------
         if(DEBUG > 0)
         {
            cout << "CALL SUCCESSFUL" << endl;
         }
      }
      else if(instr.op == 21 && instr.immediate == 1)//return
      {
         if((slice - clock) < 4) // if not enough time to complete instruction
         {
            slice = clock + 4;
         }
         
         if(DEBUG > 0)
         {
            cout << "RETURN... Stack Pointer: "<< stackPointer << endl;
            cout << "Attempting to RETURN to: " << mem[stackPointer+5] << endl;
         }
         
         if(stackPointer <= 250) //if there exists content to pull from stack
         {
            statusRegister = mem[stackPointer++]; //top of stack
            
            for(int i = 3; i >=0 ; i--)
            {
               regs[i] = mem[stackPointer++]; //push all 4 registers
            }
         
            logicalAddr = mem[stackPointer++];
            clock +=4;
         }
         else
         {
            setStatus(stack_underflow); //set status register appropriately
            clock+=4;
            return;
         }
         
         // input the logical address and return physical address through TLB lookup
         physicalAddress = convertAddressTLB(logicalAddr); 
         
         if(physicalAddress < 0) 
         { 
            // Address does not exist in memory
            pc = logicalAddr;
            setStatus(page_fault);
            return;
         }
         //Page is loaded into memory, so readjust base and limit registers accordingly
         base = (physicalAddress / FRAME_SIZE) * FRAME_SIZE;
         running->base = (logicalAddr / PAGE_SIZE) * PAGE_SIZE;
         limit = base + PAGE_SIZE;
         //set pc to physical address
         pc = physicalAddress;
         
         if(DEBUG > 0)
         {
            cout << "RETURN SUCCESSFUL" << endl;
            cout << "   PC: " << pc << endl; 
         }
      }
      else if(instr.op == 22)//read
      {
         //set status register appropriately
         setStatus(read + (instr.destReg << 3));
         
         return;
      }
      else if(instr.op == 23)//write
      {
         //set status register appropriately
         setStatus(write + (instr.destReg << 3));
         clock++;
         return;
      }
      else if(instr.op == 24)//halt
      {
         setStatus(halt); //set status register appropriately
         clock++;
           return; 
      }
      else if(instr.op == 25)//noop
      {
         clock++;
      }
      else
      {
         cout << "HALTING PROGRAM: " << running->process << endl;
         cout << "Invalid Opcode: " << instr.op << endl;
         setStatus(invalid_instruction); //set status register appropriately
         cout << "PC:" << pc-1 << "    " <<  mem[pc-1] << endl;
         return;
      }// end of instructions
   
   
      //Incremental Page Fault
      //Every other type of Page Fault is checked before this point.
      //Addresses for Jump & Call Instructions are checked during execution of the
      //instruction. If TLB lookup for those calls are successful, the pc, base 
      //and limit registers are adjusted.
      if(pc == limit)
      {
         //input logical address in TLB lookup. logical base + PAGE_SIZE
         physicalAddress = convertAddressTLB(running->base+PAGE_SIZE);
         if (physicalAddress < 0)
         {
            setStatus(page_fault); //page is not in memory: page fault
            return; 
         }
         //page is in memory, update base, limit, pc, and running->base (logical base)
         pc = physicalAddress;
         base = (physicalAddress/FRAME_SIZE) * FRAME_SIZE;
         limit = base + 8;
         //update this because we use this address for next incremental page fault
         running->base += 8; 
      }
      
      
      
      //There are only two ways in which the PC can be outside of base and limit registers.
      // 1. Incremental Page Fault
      // 2. Jump instruction. - HOWEVER, we first check to make sure that the address
      //       we are jumping to is in memory, if so, we re-adjust base and limit regs.
      //      if not, we will set a page fault and decrement the pc, so we can perform
      //      the instruction on another iteration.
      // At this point, there is no for an out of bounds error to still be present.
      if(pc >= mem.size() || pc < 0) //out-of-bounds memory reference
      {
         setStatus(out_bounds);
         return;
      }
    }//End of while loop
    
   setStatus(timeslice); //set status register appropriately
   return;
}//run

/*///////////////////////////////////////////////////////////////////////////////////////////////
*
* Function: convertAddressTLB
*
* Description: This function will go through the TLB and look for the address that the VM 
*         is trying to go to. If it is inside the TLB, then we will return the converted
*         physical address. If not, we will return -1 and allow the caller to handle it.
*
* Inputs: int logicalAddress: the logical address
*
* Outputs: int addr: the physical address in memory
*               returns (-1) if logical address is not in memory.
*
/*///////////////////////////////////////////////////////////////////////////////////////////////
int VirtualMachine::convertAddressTLB(int logicalAddress)
{
   int page = (logicalAddress/PAGE_SIZE);
   
   if(TLB.get_valid(page)) //find the physical address of the address if it is inside memory
   {
      clock+=4;
      hit++; //add one to hit count
      int frame = TLB.get_frame(page); //frame number
      int baseAddress = frame * FRAME_SIZE; //base address in memory
      int offset = (logicalAddress % FRAME_SIZE); //offset
   
      int addr = baseAddress + offset; 
      lruFrame[frame] = clock; //update lru page replacement frames
      return addr;
   }
   // page is not in memory. Return -1 to cause a page fault
   return -1;
}
/*///////////////////////////////////////////////////////////////////////////////////////////////
*
* Function: setcarry
*
* Description: This function sets the "carry" bit in the status register
*
* Inputs: none
*
* Outputs: none
*
/*///////////////////////////////////////////////////////////////////////////////////////////////
void VirtualMachine::setcarry()
{
   //if regs[instr.destReg] > 65535, then bit 16 is high which means there is a carry
   if((regs[instr.destReg] & 65536) == 65536)
   { 
   //set regs[instr.destReg] to least 16 significant bits out of 32
   regs[instr.destReg] = regs[instr.destReg] & 65535;

   statusRegister |= 1; //set carry bit to 1
   }
   else
   {
      statusRegister &= 30;
   }
}//setcarry
/*///////////////////////////////////////////////////////////////////////////////////////////////
*
* Function: setgreater
*
* Description: This function sets the "greater than" bit in the status register
*
* Inputs: none
*
* Outputs: none
*
/*///////////////////////////////////////////////////////////////////////////////////////////////
void VirtualMachine::setgreater()
{
   statusRegister |= 2; // statusRegister OR 0...00010
   statusRegister &= 19; // statusRegister AND 0...10011
}//setgreater
/*///////////////////////////////////////////////////////////////////////////////////////////////
*
* Function: setequal
*
* Description: This function sets the "equal" bit in the status register
*
* Inputs: none
*
* Outputs: none
*
/*///////////////////////////////////////////////////////////////////////////////////////////////
void VirtualMachine::setequal()
{
   statusRegister |= 4; // statusRegister OR 0...00100
   statusRegister &= 21; // statusRegister AND 0...10101
}//setequal
/*///////////////////////////////////////////////////////////////////////////////////////////////
*
* Function: setless
*
* Description: This function sets the "less than" bit in the status register
*
* Inputs: none
*
* Outputs: none
*
/*///////////////////////////////////////////////////////////////////////////////////////////////
void VirtualMachine::setless()
{
   statusRegister |= 8; // statusRegister OR 0...01000
   statusRegister &= 25; // statusRegister AND 0...11001
}//setless
/*///////////////////////////////////////////////////////////////////////////////////////////////
*
* Function: setoverflow
*
* Description: This function sets the "overflow" bit in the status register
*
* Inputs: none
*
* Outputs: none
*
/*///////////////////////////////////////////////////////////////////////////////////////////////
void VirtualMachine::setoverflow(int x, int y, int result)
{
   int x15 = x15 >> 15;; //bit 15 of integer x
   int y15 = y15 >> 15;; //bit 15 of integer y
   int result15 = result15 >> 15;;  //bit 15 of integer result

   //checks if instr.destReg and instr.sourceReg are positive while result is negative OR 
// instr.destReg and instr.sourceReg are negative while result is positive
   if((x15 == 0 && y15 == 0 && result15 == 1) || (x15 == 1 && y15 == 1 && result15 == 0))
   {
      statusRegister |= 16; // statusRegister OR 0...10000
   }
   else
   {
   statusRegister &= 15;
   }
}//setoverflow
/*///////////////////////////////////////////////////////////////////////////////////////////////
*
* Function: setStatus
*
* Description: This function set the status bits in the status register to the inputted value
*
* Inputs: return_status: the status we want to put in the status register
*
* Outputs: none
*
/*///////////////////////////////////////////////////////////////////////////////////////////////
void VirtualMachine::setStatus(int return_status)
{   
   return_status <<= 5; //shift return status to appropriate bits
   statusRegister |= return_status; //set status
   statusRegister &= (return_status + 31); //clear any previous status
}
/*///////////////////////////////////////////////////////////////////////////////////////////////
*
* Function: carry
*
* Description: This function will return the "carry" bit
*
* Inputs: int logicalAddress: the logical address
*
* Outputs: int carry: the value of the "carry" bit. High or Low
*
/*///////////////////////////////////////////////////////////////////////////////////////////////
int VirtualMachine::carry()
{
   int carry;
   carry = statusRegister & 1; //get status of bit 0 (no need to shift)
   return carry; //return carry (1st) bit of status register
}//carry
/*///////////////////////////////////////////////////////////////////////////////////////////////
*
* Function: is_greater
*
* Description: This function will return the "greater than" bit
*
* Inputs: none
*
* Outputs: int greater: the value of the "greater than" bit. High or Low
*
/*///////////////////////////////////////////////////////////////////////////////////////////////
int VirtualMachine::is_greater()
{
   int greater; 
   greater = statusRegister & 2; // get bit 1
   greater = greater >> 1; //shift right
   return greater; //return greater bit of status register
}//greater
/*///////////////////////////////////////////////////////////////////////////////////////////////
*
* Function: is_equal
*
* Description: This function will return the "equal" bit
*
* Inputs: none
*
* Outputs: int equal: the value of the "equal" bit. High or Low
*
/*///////////////////////////////////////////////////////////////////////////////////////////////
int VirtualMachine::is_equal()
{
   int equal;
   equal = statusRegister & 4; //get bit 2
   equal = statusRegister >> 2; //shift right
   return equal; //return equal bit of status register
}//equal
/*///////////////////////////////////////////////////////////////////////////////////////////////
*
* Function: is_less
*
* Description: This function will return the "less than" bit
*
* Inputs: none
*
* Outputs: int less: the value of the "less than" bit. High or Low
*
/*///////////////////////////////////////////////////////////////////////////////////////////////
int VirtualMachine::is_less()
{
   int less;
   less = statusRegister & 8; // get bit 3
   less = less >> 3; //shift right
   return less; //return less bit of status register
}//less
/*///////////////////////////////////////////////////////////////////////////////////////////////
*
* Function: overflow
*
* Description: This function will return the "overflow" bit
*
* Inputs: none
*
* Outputs: int over: the value of the "overflow"  bit. High or Low
*
/*///////////////////////////////////////////////////////////////////////////////////////////////
int VirtualMachine::overflow()
{
   int over;
   over = statusRegister & 16; //get bit 4
   over = over >> 4; //shift right
   return over; //return overflow bit of status register
}//overflow