#include "VirtualMachine.h"
#include "Assembler.h"

/***************************************************************
 * Name(s): Jacob LeReaux, Michael Kang 
 * File: os.cpp
 * Date: April 4, 2016 - April 18, 2016
 * Description: This code is an implementation of the Operating System (OS).
 * The os.cpp file brings together the code from the Assembler and Virtual Machine and test
 * whether it can properly link together the Asssembler and Virtual Machine and perform
 * its task.
 ****************************************************************/

using namespace std;

int main(int argc, char *argv[])
{
	string arg = argv[1];
	//".s"
	fstream assemblefile;
	assemblefile.open(arg.c_str(),ios::in); //open file for input
	//".o"
	string object = arg.replace(arg.find('.'),arg.length(),".o");
	fstream objectfile;
	objectfile.open(object.c_str(),ios::out);
	
	//////Assemble///////
	Assembler as;
	as.assemble(assemblefile,objectfile); 
	
	//close files
	assemblefile.close();
	objectfile.close();
	

   //open input file
	fstream inputfile;
	string input = arg.replace(arg.find('.'),arg.length(),".in");
	inputfile.open(input.c_str(),ios::in);

	//open output file for clock and "write"
	fstream outputfile;      
	string output = arg.replace(arg.find('.'),arg.length(),".out");
	outputfile.open(output.c_str(), ios::out);

	//open objectfile      
	objectfile.open(object.c_str(),ios::in | ios::out);
	

	//run virtualmachine
	VirtualMachine vm;
	vm.run(objectfile,inputfile,outputfile);

	objectfile.close();
	inputfile.close();
	outputfile.close();
	return 0;
}//main
