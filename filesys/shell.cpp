
#include "sdisk.h"
#include "filesys.h"
#include "shell.h"
#include "block.cpp"

Shell::Shell(string diskname, int numberofblocks, int blocksize): Filesys(diskname,numberofblocks,blocksize)
{
   this->diskname = diskname;
   this->blocksize = blocksize;
   this->numberofblocks = numberofblocks;
}
int Shell::dir()// lists all files
{
   vector<string> files = ls();
   if(files.size() == 0)
   {
      return 0;
   }
   for(int i = 0; i < files.size(); i++)
   {
      if(files[i] != "xxxxx")
      {
         cout << files[i] << "  ";
      }   
   }
   cout << endl;
   return 1;
}
int Shell::add(string file)// add a new file using input from the keyboard
{
   int blockid = getfirstblock(file);
   if(blockid >= 0)
   {
      cout << file << " already exists" << endl;
      return 0;
   } 
   else
   {
      newfile(file);
      cout << "Enter Contents of File: " << endl;
      string buffer;
      char x; 
      while(x != '~')
      {
         cin.get(x);
         buffer += x;
      }
      vector<string> blocks = block(buffer, blocksize);
      for(int i = 0; i < blocks.size(); i++)
      {
         addblock(file, blocks[i]);
      }   
      return 1;   
   }
}
int Shell::del(string file)// deletes the file
{
   int firstblock = getfirstblock(file);
   if(firstblock == -1)
   {
      return -1;
   }
   else if(firstblock == 0)
   {
      rmfile(file);
      return 1;
   }
   else
   {
      while(firstblock != 0)
      {
         delblock(file,firstblock);
         firstblock = getfirstblock(file);
      }
      rmfile(file);
   }
   return 1;
}
int Shell::type(string file)//lists the contents of file
{
   int block = getfirstblock(file);
   if(block == -1) //no file
   {
      cout << file << " does not exist" << endl;
      return block;
   }
   else if(block == 0) //no blocks
   {
      cout << file << " is an empty file" << endl;
      return block;
   }
   else // there is data on the file
   {
      string content;
      while(block != 0)
      {   
         string buffer;
         readblock(file, block, buffer); //read block to buffer
         content += buffer; //add buffer to string
         block = nextblock(file, block); // go to next block 
      }
      cout << content.substr(0,content.find('~')) << endl; //cout content
      return 1;
   }
}
int Shell::copy(string file1, string file2)//copies file1 to file2
{
   int block = getfirstblock(file1);
   if(block == -1)
   {
      cout << file1 << " does not exist" << endl;
      return block;
   }
   if(getfirstblock(file2) >= 0)
   {
      cout << file2 << " already exists" << endl;
      return 0; 
   }
   newfile(file2);
   while(block != 0)
   {
      string buffer;
      readblock(file1, block, buffer); //read block into buffer
      addblock(file2, buffer); //add block containing buffer
      block = nextblock(file1, block); //move to next block
   }
   return 1;  
}

