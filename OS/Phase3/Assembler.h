#ifndef ASSEMBLER_H
#define ASSEMBLER_H

/***************************************************************
 * Name(s): Jacob Lereaux, Michael Kang 
 * File: Assembler.h
 * Date: April 4, 2016 - April 18, 2016
 * Description: This code is an implementation of the Assembler. The Assembler
 * reads in a '.s' file which contains assembly source code and it translate the
 * source code into binary code in which the Virtual Machine can read and output an
 * '.o' file in which is inputted into the Virtual Machine.
 ****************************************************************/

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>

using namespace std;

class Assembler 
{
   private:
   string op;
   int rd;
   int i;
   int rs;
   int con;
   int addr;
   int opcode;
   int instr;
   vector<int> obj;
   
   public:
   Assembler(); //constructor
   void assemble(fstream &assembly, fstream &objectfile); //loads memory vector then performs instr-fetch-execute
   void objectfile();
   
    //////Checks if instructions are good/////////
    void checkrd();
   void checkrs();
   void checkaddr();
   void checkcon();
};

#endif
