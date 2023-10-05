#include "Assembler.h"

/***************************************************************
 * Name(s): Jacob Lereaux, Michael Kang 
 * File: Assembler.cpp
 * Date: April 4, 2016 - April 18, 2016
 * Description: This code is an implementation of the Assembler. The Assembler
 * reads in a '.s' file which contains assembly source code and it translate the
 * source code into binary code in which the Virtual Machine can read and output an
 * '.o' file in which is inputted into the Virtual Machine.
 ****************************************************************/


Assembler::Assembler()
{
   instr = 0;
   rd = 0;
   rs = 0;
   i = 0;
   con = 0;
   addr = 0;
   opcode = 0;
}//Assembler()

void Assembler::assemble(fstream &assembly,fstream &objectfile)
{
   if (!assembly.is_open())
   {
      cout << ".s failed to open.\n";
      exit(1);
   }
    
   string line, op;

   getline(assembly, line);

   while(!assembly.eof()) //(assembly.good()) 
   {

      if(line.find('!') == 0 or line.find(' ') == 0)
         {
         getline(assembly, line);
         }

         if(line.find('!') <  line.length()) //if there exits a comment
         {
         line.erase(line.find('!'), line.length()); //erase the comment
         }

      istringstream str(line.c_str());
      // clear values;
      rd = 0; 
      i = 0;
      rs = 0;
      addr = 0; 
      con = 0;
      str >> op;
      
      if(op == "load")
      {
         str >> rd >> addr;
         opcode = 0;
         i = 0;
         checkrd();
         checkaddr();
         opcode <<= 11;
         rd <<= 9;
         i <<= 8;
         instr = opcode + rd + i + addr;
      }
      else if(op == "loadi")
      {
         str >> rd >> con;
         opcode = 0;
         i = 1;
         checkrd();
         checkcon();
         opcode <<= 11;
         rd <<= 9;
         i <<= 8;
         instr = opcode + rd + i + con;
      }
      else if(op == "store")
      {
         str >> rd >> addr;
         opcode = 1;
         i = 1;
         checkrd();
         checkaddr();
         opcode <<= 11;
         rd <<= 9;
         i <<= 8;
         instr = opcode + rd + i + addr;   
      }
      else if(op == "add")
      {
         str >> rd >> rs;
         opcode = 2;
         i = 0;
         checkrd();
         checkrs();
         opcode <<= 11;
         rd <<= 9;
         i <<= 8;
         rs <<= 6;
         instr = opcode + rd + i + rs;
      }
      else if(op == "addi")
      {
         str >> rd >> con;
         opcode = 2;
         i = 1;
         checkrd();
         checkcon();
         opcode <<= 11;
         rd <<= 9;
         i <<= 8;
         instr = opcode + rd + i + con;
      }
      else if(op == "addc")
      {
         str >> rd >> rs;
         opcode = 3;
         i = 0;
         checkrd();
         checkrs();
         opcode <<= 11;
         rd <<= 9;
         rs <<= 6;
         instr = opcode + rd + i + rs;
      }
      else if(op == "addci")
      {
         str >> rd >> con;
         opcode = 3;
         i = 1;
         checkrd();
         checkcon();
         opcode <<= 11;
         rd <<= 9;
         i <<= 8;
         instr = opcode + rd + i + con;
      }
      else if(op == "sub")
      {
         str >> rd >> rs;
         opcode = 4;
         i = 0;
         checkrd();
         checkrs();
         opcode <<= 11;
         rd <<= 9;
         i <<= 8;
         rs <<= 6;
         instr = opcode + rd + i + rs;
      }
      else if(op == "subi")
      {
         str >> rd >> con;
         opcode = 4;
         i = 1;
         checkrd();
         checkcon();
         opcode <<= 11;
         rd <<= 9;
         i <<= 8;
         instr = opcode + rd + i + con;
      }
      else if(op == "subc")
      {
         str >> rd >> rs;
         opcode = 5;
         i = 0;
         checkrd();
         checkrs();
         opcode <<= 11;
         rd <<= 9;
         i <<= 8;
         rs <<= 6;
         instr = opcode + rd + i + rs;
      }
      else if(op == "subci")
      {
         str >> rd >> con;
         opcode = 5;
         i = 1;
         checkrd();
         checkcon();
         opcode <<= 11;
         rd <<= 9;
         i <<= 8;
         instr = opcode + rd + i + con;
      }
      else if(op == "and")
      {
         str >> rd >> rs;
         opcode = 6;
         i = 0;
         checkrd();
         checkrs();
         opcode <<= 11;
         rd <<= 9;
         i <<= 8;
         rs <<= 6;
         instr = opcode + rd + rs;
      }
      else if(op == "andi")
      {
         str >> rd >> con;
         opcode = 6;
         i = 1;
         checkrd();
         checkcon();
         opcode <<= 11;
         rd <<= 9;
         i <<= 8;
         instr = opcode + rd + i + con;
      }
      else if(op == "xor")
      {
         str >> rd >> rs;
         opcode = 7;
         i = 0;
         checkrd();
         checkrs();
         opcode <<= 11;
         rd <<= 9;
         i <<= 8;
         rs <<= 6;
         instr = opcode + rd + i + rs;
      }
      else if(op == "xori")
      {
         str >> rd >> con;
         opcode = 7;
         i = 1;
         checkrd();
         checkcon();
         opcode <<= 11;
         rd <<= 9;
         i << 8;
         instr = opcode + rd + i + con;
      }
      else if(op == "compl")
      {
         str >> rd;
         opcode = 8;
         checkrd();
         opcode <<= 11;
         rd <<= 9;
         instr = opcode + rd;
      }
      else if(op == "shl")
      {
         str >> rd;
         opcode = 9;
         checkrd();
         opcode <<= 11;
         rd <<= 9;
         instr = opcode + rd;
      }
      else if(op == "shla")
      {
         str >> rd;
         opcode = 10;
         checkrd();
         opcode <<= 11;
         rd <<= 9;
         instr = opcode + rd;
      }
      else if(op == "shr")
      {
         str >> rd;
         opcode = 11;
         checkrd();
         opcode <<= 11;
         rd <<= 9;
         instr = opcode + rd;
      }
      else if(op == "shra")
      {
         str >> rd;
         opcode = 12;
         checkrd();
         opcode <<= 11;
         rd <<= 9;
         instr = opcode + rd;
      }
      else if(op == "compr")
      {
         str >> rd >> rs;
         opcode = 13;
         i = 0;
         checkrd();
         checkrs();
         opcode <<= 11;
         rd <<= 9;
         i <<= 8;
         rs <<= 6;
         instr = opcode + rd + i + rs;
      }
      else if(op == "compri")
      {
         str >> rd >> con;
         opcode = 13;
         i = 1;
         checkrd();
         checkcon();
         opcode <<= 11;
         rd <<= 9;
         i <<= 8;
         instr = opcode + rd + i + con;
      }
      else if(op == "getstat")
      {
         str >> rd;
         opcode = 14;
         checkrd();
         opcode <<= 11;
         rd <<= 9;
         instr = opcode + rd;
      }
      else if(op == "putstat")
      {
         str >> rd;
         opcode = 15;
         checkrd();
         opcode <<= 11;
         rd <<= 9;
         instr = opcode + rd;
      }
      else if(op == "jump")
      {
         str >> addr;
         opcode = 16;
         i = 1;
         checkaddr();
         opcode <<= 11;
         i <<= 8;
         instr = opcode + i + addr;
      }
      else if(op == "jumpl")
      {
         str >> addr;
         opcode = 17;
         i = 1;
         checkaddr();
         opcode <<= 11;
         i <<= 8;
         instr = opcode + i + addr;
      }
      else if(op == "jumpe")
      {
         str >> addr;
         opcode = 18;
         i = 1;
         checkaddr();
         opcode <<= 11;
         i <<= 8;
         instr = opcode + i + addr;
      }
      else if(op == "jumpg")
      {
         str >> addr;
         opcode = 19;
         i = 1;
         checkaddr();
         opcode <<= 11;
         i <<= 8;
         instr = opcode + i + addr;
      }
      else if(op == "call")
      {
         opcode = 20;
         i = 1;
         str >> addr;
         checkaddr();
         opcode <<= 11;
         i <<= 8;
         instr = opcode + i + addr;
      }
      else if(op == "return")
      {
         opcode = 21;
         i = 1;
         opcode <<= 11;
         i <<= 8;
         instr = opcode + i;
      }
      else if(op == "read")
      {
         str >> rd;
         opcode = 22;
         checkrd();
         opcode <<= 11;
         rd <<= 9;
         instr = opcode + rd;
      }
      else if(op == "write")
      {
         str >> rd;
         opcode = 23;
         checkrd();
         opcode <<= 11;
         rd <<= 9;
         instr = opcode + rd;
      }
      else if(op == "halt")
      { 
         opcode = 24;
         opcode <<= 11;
         instr = opcode;

         
      }
      else if(op == "noop")
      {
         opcode = 25;
         opcode <<= 11;
         instr = opcode;
      }
      else
      {
         cout << op << "###" << endl;
         cout << "Invalid Command" << endl;
         exit(0);
      }
   
      obj.push_back(instr);   
      getline(assembly, line);
      


   }//End of While Loop

   /////write to objectfile//////
   objectfile << obj[0] << endl;
   
   for(int i = 1; i < obj.size(); i++)
   { 
      objectfile << obj[i] << endl;
   }

   obj.clear(); //clears content of vector to prevent overlap of file instructions
}//assemble

void Assembler::checkrd()
{
   if(rd <= 3 and rd >= 0)
   {
      return;
   }

   cout << "Invalid Range for Destination Register" << endl;
   exit(0);
}//checkrd

void Assembler::checkrs()
{
   if(rs <= 3 and rs >= 0)
   {
      return;
   }

   cout << "Invalid Range for Source Register" << endl;
   exit(0);
}//checkrs

void Assembler::checkaddr()
{
   if(addr >= 0 and addr < 256)
   {
      return;
   }

   cout << "Invalid Range for Address" << endl;
   exit(0);
}//checkaddr

void Assembler::checkcon()
{
   // checks if constant is within range
   if(con >= -128 and con < 128)
   {
      if(con >= -128 and con < 0)
      {
         con |= 128;
      }

      con &= 255; //gets rid of all the ones past 8 bits
      return;
   }

   cout << "Invalid Range for Constant Value" << endl;
   exit(0);
}//checkcon
