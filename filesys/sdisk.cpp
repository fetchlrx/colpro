// Jacob LeReaux - 004961563'
// Lab 2
// 4/19/16

#include "sdisk.h"

Sdisk::Sdisk(string diskname, int numberofblocks, int blocksize)
{
   this->diskname = diskname; //set diskname
   this->numberofblocks = numberofblocks; //set number of blocks
   this->blocksize = blocksize; //set blocksize

   fstream disk(diskname.c_str());   
   if(!disk.good()) //if file does not exist
   {
      disk.open(diskname.c_str(), ios::out); //create it
      for(int i = 0; i < (numberofblocks * blocksize); i++)
      {
         disk.put('#'); //memory
      }
      disk.close();
   }
}
int Sdisk::getblock(int blocknumber, string& buffer)
{
   
   fstream infile; //fstream
   infile.open(diskname.c_str(), ios::in); //open diskname
   infile.seekg(blocknumber*blocksize); //move get pointer 
   char c; //initialize character c
   infile.get(c); //load c with value of get pointer (after, get pointer += 1)
   int i = 0; 
   while(infile.good() && i < blocksize) //while file is good and loop blocksize times
   {
      buffer += c; //add char c to string buffer
      infile.get(c); //load c with new value and move pointer
      i++; //increment
   }

   infile.seekg(blocknumber*blocksize); //move get pointer to beginning of block
   infile.get(c); //load c
   //check character by character if string buffer matches the position of get pointer
   for(int i = 0; i < buffer.size(); i++)
   {
      if(buffer[i] != c)
      {
         return 0; //failed
      }
      infile.get(c);
   }
   infile.close();
   return 1; //success
}
int Sdisk::putblock(int blocknumber, string buffer)
{
   //if string is larger than blocksize, then cannot put
   if(buffer.length() > blocksize)
   {
      return 0;
   }

   fstream outfile; //fstream
   outfile.open(diskname.c_str(), ios::in | ios::out); //open file and delcare it as both input and output
   outfile.seekp(blocknumber*blocksize); //move put pointer
   int i = 0;
   while(outfile.good() && i < blocksize) //while file is good and loop blocksize times
   {
      outfile.put(buffer[i]); //write character (position i) to file (after, move put pointer by one)
      i++;
   }
   outfile.close();
   
   string test; //test if success
   getblock(blocknumber, test); //load string test with block blocknumber
   if(test == buffer) //if string test = string buffer
   {
      return 1; //success
   }  
   else
   {
      return 0; //failure
   }
}
int Sdisk::getnumberofblocks()
{
   return numberofblocks;
}
int Sdisk::getblocksize()
{
   return blocksize;
}







