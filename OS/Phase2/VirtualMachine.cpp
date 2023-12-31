#include "VirtualMachine.h"

/***************************************************************
 * Name(s): Jacob Lereaux, Michael Kang 
 * File: VirtualMachine.cpp
 * Date: April 4, 2016 - April 18, 2016
 * Description: This code is an implementation of a 16-bit Virtual Machine (VM).
 * The VM reads the '.o' file from the Assembler and perform the following instruction
 * from the given instructions.
 ****************************************************************/

VirtualMachine::VirtualMachine()
{
   r = vector<int>(REG_FILE_SIZE);
   mem = vector<int>(MEM_SIZE);
   pc = 0;
   ir = 0;
   sr = 0;
   sp = 256;
   limit = 0; 
   base = 0;
   clock = 0;
   op = 0;
   rd = 0;
   i = 0;
   rs = 0;
   con = 0;
   addr = 0;
   instr = 0; //keeps track of instructions being inputted into memory
}//VirtualMachine()
void VirtualMachine::load(fstream &objectfile, PCB* p)
{
   if (!objectfile.is_open())
   {
      cout << "prog.s failed to open.\n";
      exit(1);
   }   
   base = base+limit;
   objectfile >> mem[instr];
   limit = 0;
   while(!objectfile.eof()) 
   {
      limit++;
      instr++;
      objectfile >> mem[instr];
   }
   pc = base;
   p->set_data(pc,base,limit,sr,r,sp);
}//load
/*//////////////////////////////////////////////////////////////
*   FUNCTION: run()
*
*   DESCRIPTION: Executes current program indicated through inputed Program Control Block (PCB). Uses bit extraction to gather 
            OP_CODE, Registers (Source and Destination), IMMEDIATE VALUE, to perform instruction inside IF ELSE IF BLOCK.
            Executes until timeslice is reached, IO has been requested, or a halt instruction. Will then return to OS.
            OS handles all context switching and IO requests and places PCB in appropriate queue (Wait or Ready).
            Contains status register that is updated when status changes, which OS has access to.

Instruction Register:
            -------------------------------------------------------------------------------------------------------
* BIT:         |  15  |  14  |  13  |  12  |  11  |  10  |  9  |  8  |  7  |  6  |  5  |  4  |  3  |  2  |  1  |  0  |
*            -------------------------------------------------------------------------------------------------------
* Instr:       |             OP_CODE             |  Dest_Reg  |  I  |Source_Reg |            DONT CARE              |
*            -------------------------------------------------------------------------------------------------------
* Immediate:   |             OP_CODE              |  Dest_Reg  |  I  |   IMMEDIATE VALUE  (CONSTANT OR ADDRESS)     |
*            -------------------------------------------------------------------------------------------------------
* BIT:         |  15  |  14  |  13  |  12  |  11  |  10  |  9  |  8  |  7  |  6  |  5  |  4  |  3  |  2  |  1  |  0  |
*            -------------------------------------------------------------------------------------------------------
*
*///////////////////////////////////////////////////////////////
void VirtualMachine::run(PCB* running)
{
   
   int incr;
   clock_start = clock;
   pc = running->pc;
   //endtime = alloted time for timeslice (15 clock ticks)
   int slice = clock + timesliceAmount;
   ////// fetch-execute cycle /////////
   while(pc < (base + limit) && clock < slice)  
   {
      ir = mem[pc]; //fetch instruction from memory
      pc++;  //move to next location
      op = ir >> 11; //set opcode
   
      i = ir & i_mask; //set immediate
      i >>= 8;
   
      rd = ir & rd_mask; //set destination register
      rd >>= 9;
   
      rs = ir & rs_mask; //set source register
      rs >>= 6;
   
      con = ir & const_mask; //set constant
      
      if(con > 127) // if constant is greater than 127, sign bit will be high
      {
         con |= 65280; //sign extend to 16 bits 
      }
      addr = ir & addr_mask; //set address
      addr += base; // addresses are offset of base

      //Execution codes
      if(op == 0 && i == 0)//load
      {
         if((slice - clock) < 4) // if not enough time to complete instruction
         {
            slice = clock + 4; // extend timeslice
         }      
         checkrd();
         checkaddr();
         r[rd] = mem[addr];
         clock += 4;
      }
      else if(op == 0 && i == 1)//loadi
      {
         checkrd();
         checkcon();
         r[rd] = con;
         clock++;
      }
      else if(op == 1)//store
      {
         if((slice - clock) < 4)// if not enough time to complete instruction
         {
            slice = clock + 4;
         }      
         checkrd();
         checkaddr();
         mem[addr] = r[rd];
         clock += 4;
      }
      else if(op == 2 && i == 0)//add
      {
         checkrd();
         checkrs();
         //set overflow by checking both numbers and their sum
         setoverflow(r[rd],r[rs],r[rd] + r[rs]);
         r[rd] += r[rs];
         setcarry();
         clock++;
      }
      else if(op == 2 && i == 1)//addi
      {
         checkrd();
         checkcon();
         //set overflow by checking both numbers and their sum
         setoverflow(r[rd],con,r[rd] + con); 
         r[rd] += con;
         setcarry(); 
         clock++;
      }
      else if(op == 3 && i == 0)//addc
      {
         checkrd();
         checkrs();
         //set overflow by checking both numbers and their sum
         setoverflow(r[rd],r[rs] + carry(),r[rd] + r[rs] + carry());
         r[rd] += r[rs] + carry();
         setcarry();
         clock++;
      }
      else if(op == 3 && i == 1)//addci
      {
         checkrd();
         checkcon();
         setoverflow(r[rd],con + carry(),r[rd] + con + carry()); //set overflow by checking both numbers and their sum
         r[rd] += con + carry();
         setcarry();
         clock++;
      } 
      else if(op == 4 && i == 0)//sub
      {
         checkrd();
         checkrs();
         //////overflow?  (+) - (-)  --> (+) + (+)  
         r[rd] -= r[rs];
         setcarry();
         clock++;
      }
      else if(op == 4 && i == 1)//subi
      {
         checkrd();
         checkcon();
         //overflow??
         r[rd] -= con;
         setcarry();
         clock++;
      }
      else if(op == 5 && i == 0)//subc
      {
         checkrd();
         checkrs();
         //overflow?
         r[rd] -= r[rs] - carry();
         setcarry();
         clock++;   
      }
      else if(op == 5 && i == 1)//subci
      {
         checkrd();
         checkrs();
         //overflow?
         r[rd] = r[rd] - con - carry();
         setcarry();
         clock++;
      }
      else if(op == 6 && i == 0)//and
      {
         checkrd();
         checkrs();
         r[rd] &= r[rs];
         clock++;
      }
      else if(op == 6 && i == 1)//andi
      {
         checkrd();
         checkcon();
         r[rd] &= con;
         clock++;
      }
      else if(op == 7 && i == 0)//xor
      {
         checkrd();
         checkrs();
         rd ^= r[rs];
         clock++;
      }
      else if(op == 7 && i == 1)//xori
      {
         checkrd();
         checkcon();
         rd ^= con;
         clock++;
      }
      else if(op == 8)//compl
      {
         checkrd();
         rd ^= 65553; //xor with 1111111111111111 to get compl
         clock++;
      }
      else if(op == 9)//shl
      {
         checkrd();
         setcarry();

         r[rd] = r[rd] << 1;
         clock++;
         
         if(r[rd] > 65535) 
         {
            setcarry();
         }
      }
      else if(op == 10)//shla
      {
         checkrd();
         setcarry();
         if(r[rd] > 32767) 
         {
            r[rd] = r[rd] << 1;   
            r[rd] |= 32768;   
         }
         else
         {   
            r[rd] = r[rd] << 1;   
         }
         
         clock++;
      }
      else if(op == 11)//shr
      {
         checkrd();
         if((r[rd] & 1) == 1)
         {
               sr |= 1;
         }
         else
         {
               sr &= 30;
         }
         r[rd] = r[rd] >> 1;
         clock++;
      }
      else if(op == 12)//shra
      {
         checkrd();
         
         if((r[rd] & 1) == 1) 
         {
            sr |= 1;
         }
         else
         {
            sr &= 30;
         }

         if(r[rd] > 32767)
         {
            r[rd] = r[rd] >> 1;   
            r[rd] |= 32768;   
         }
         else
         {   
            r[rd] = r[rd] >> 1;   
         }

         clock++;
      }
      else if(op == 13 && i == 0)//compr
      {
         checkrd();
         checkrs();
         
         if(r[rd] < r[rs])
         {
            setless();
         }
         else if(r[rd] == r[rs])
         {
            setequal();
         }
         else if(r[rd] > r[rs])
         {
            setgreater();
         }
         
         clock++;
      }
      else if(op == 13 && i == 1)//compri
      {
         checkrd();
         checkcon();
         
         if(r[rd] < con)
         {
            setless();
         }
         else if(r[rd] == con)
         {
            setequal();
         }
         else if(r[rd] > con)
         {
            setgreater();
         } 
         clock++;
         }
         else if(op == 14)//getstat
         {
         checkrd();
         r[rd] = sr;
         clock++;
      }
      else if(op == 15)//putstat
      {
         checkrd();
         sr = r[rd];
         sr &= 31;
         clock++;
      }
      else if(op == 16 && i == 1)//jump
      {
         checkrd();
         checkaddr();
         pc = addr;
         clock++;
      }
      else if(op == 17 && i == 1)//jumpl
      {
         checkrd();
         checkaddr();
         
         if(less() == 1)
         {
            pc = addr;
         }
         
         clock++;
      }
      else if(op == 18 && i == 1)//jumpe
      {
         checkrd();
         checkaddr();
         
         if(equal() == 1)
         {
            pc = addr;
         }
         
         clock++;
      }
      else if(op == 19 && i == 1)//jumpg
      {
         if(greater() == 1)
         {
            pc = addr;
         }
         
         clock++;
      }
      else if(op == 20 && i == 1)//call
      {
         if((slice - clock) < 4) // if not enough time to complete instruction
         {
            slice = clock + 4;
         }         

         checkrd();
         checkaddr();

         if(sp > ((base+limit) + 6)) //if there are at least 6 open memory spaces between top of stack and limit;
         {
            mem[--sp] = pc; //push program counter
               
            for(int i = 0; i < 4; i++)
            {
               mem[--sp] = r[i]; //push all 4 registers
            }
            mem[--sp] = sr; //push status register
            clock += 4;
         }
         else
         { 
            setreturn(stack_overflow); //set status register appropriately
            clock += 4;
            return;
         }   
         
         pc = addr;      
      }
      else if(op == 21 && i == 1)//return
      {
         if((slice - clock) < 4) // if not enough time to complete instruction
         {
            slice = clock + 4;
         }         
         
         if(sp <= 250) //if there exists content to pull from stack
         {
            sr = mem[sp++]; //top of stack
         
            for(int i = 3; i >=0 ; i--)
            {
               r[i] = mem[sp++]; //push all 4 registers
            }
            pc = mem[sp++];
            clock +=4;
         }
         else
         {
            setreturn(stack_underflow); //set status register appropriately
            clock+=4;
            return;
         }
      }
      else if(op == 22)//read
      {
         //set return status and have OS handle Input request
         checkrd();
         clock++;
         setreturn(read + (rd << 3)); //set status register appropriately
         return;
      }
      else if(op == 23)//write
      {
         //set return status and have OS handle output request
         checkrd();
         if(r[rd] > 32767)
         {   
            int temp;
            temp = r[rd] | 4294901760; //sign extend to 32 bits
            // output << temp << endl;
         }   
         else
         {
            
         }
         setreturn(write + (rd << 3)); //set status register appropriately
         clock++;
         return;
      }
      else if(op == 24)//halt
      {
         setreturn(halt); //set status register appropriately
         clock++;
         return; 
      }
      else if(op == 25)//noop
      {
         clock++;
      }
      else
      {
         setreturn(invalid_opcode); //set status register appropriately
         return;
      }
      incr++;
   }//End of while loop
    
   if(pc >= (base+limit) && pc < base) //out-of-bounds reference
   {
      setreturn(out_bounds); //set status register appropriately
      return;
   }
   
   setreturn(timeslice); //set status register appropriately
   return;
}//run

void VirtualMachine::checkrd()
{
   if(rd <= 3 && rd >= 0)
   {
      return;
   }

   cout << "Invalid Range for Destination Register" << endl;
}//checkrd

void VirtualMachine::checkrs()
{
   if(rs <= 3 && rs >= 0)
   {
      return;
   }

   cout << "Invalid Range for Source Register" << endl;
}//checkrs

void VirtualMachine::checkaddr()
{
   if(addr >= 0 && addr < (base+limit))
   { 
      return;
   }

   cout << "Invalid Range for Address" << endl;
}//checkaddr

void VirtualMachine::checkcon()
{
   // checks if constant is within range
   if((con >= 0 && con < 128) || (con <= 65535 && con >= 65280))
   { 
      return;
   }

   cout << "Invalid Range for Constant Value" << endl;
}//checkcon

void VirtualMachine::setcarry()
{
   if((r[rd] & 65536) == 65536) //if r[rd] > 65535, then bit 16 is high which means there is a carry
   { 
      r[rd] = r[rd] & 65535; //set r[rd] to least 16 significant bits out of 32
      sr |= 1; //set carry bit to 1
   }
   else
   {
      sr &= 30;
   }
}//setcarry

void VirtualMachine::setgreater()
{
   sr |= 2; // sr OR 0...00010
   sr &= 19; // sr AND 0...10011
}//setgreater

void VirtualMachine::setequal()
{
   sr |= 4; // sr OR 0...00100
   sr &= 21; // sr AND 0...10101
}//setequal

void VirtualMachine::setless()
{
   sr |= 8; // sr OR 0...01000
   sr &= 25; // sr AND 0...11001
}//setless

void VirtualMachine::setoverflow(int x, int y, int result)
{
   int x15 = x15 >> 15;; //bit 15 of integer x
   int y15 = y15 >> 15;; //bit 15 of integer y
   int result15 = result15 >> 15;;  //bit 15 of integer result

   //checks if rd and rs are positive while result is negative OR rd and rs are negative while result is positive
   if((x15 == 0 && y15 == 0 && result15 == 1) || (x15 == 1 && y15 == 1 && result15 == 0))
   {
      sr |= 16; // sr OR 0...10000
   }
   else
   {
   sr &= 15;
   }
}//setoverflow
void VirtualMachine::setreturn(int return_status)
{   
   return_status <<= 5; //shift return status to appropriate bits
   sr |= return_status; //set status
   sr &= (return_status + 31); //clear any previous status
}
int VirtualMachine::carry()
{
   int carry;
   carry = sr & 1; //get status of bit 0 (no need to shift)
   return carry; //return carry (1st) bit of status register
}//carry

int VirtualMachine::greater()
{
   int greater; 
   greater = sr & 2; // get bit 1
   greater = greater >> 1; //shift right
   return greater; //return greater bit of status register
}//greater

int VirtualMachine::equal()
{
   int equal;
   equal = sr & 4; //get bit 2
   equal = sr >> 2; //shift right
   return equal; //return equal bit of status register
}//equal

int VirtualMachine::less()
{
   int less;
   less = sr & 8; // get bit 3
   less = less >> 3; //shift right
   return less; //return less bit of status register
}//less

int VirtualMachine::overflow()
{
   int over;
   over = sr & 16; //get bit 4
   over = over >> 4; //shift right
   return over; //return overflow bit of status register
}//overflow

