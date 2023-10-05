//Jacob LeReaux - 004961563
//Lab 5 - CSE461 Tuesday
//5-10-16



#include "sdisk.h"
#include "filesys.h"

vector<string> block(string buffer, int b)
{
   // blocks the buffer into a list of blocks of size b

   vector<string> blocks;
   int numberofblocks=0;

   if (buffer.length() % b == 0) 
   { 
      numberofblocks= buffer.length()/b;
   } 
   else 
   { 
      numberofblocks= buffer.length()/b +1;
   }

   string tempblock;
   for (int i=0; i<numberofblocks; i++) 
   { 
      tempblock=buffer.substr(b*i,b);
      blocks.push_back(tempblock);
   }

   int lastblock=blocks.size()-1;

   for (int i=blocks[lastblock].length(); i<b; i++) 
   { 
      blocks[lastblock]+="#";
   }

   return blocks;
}

Filesys::Filesys(string diskname, int numberofblocks, int blocksize): Sdisk(diskname,numberofblocks,blocksize)
{
      rootsize = getblocksize() / 12;
      fatsize = ((getnumberofblocks() * 6) / getblocksize() ) + 1 ;

      string buffer;
      getblock(0,buffer);
   if(buffer[0] == '#')
   {   //no file system build root and fat
      ///Build Root
      for(int i = 1; i <= rootsize; i++)
      {
            filename.push_back("xxxxx"); // push back "no file" identifiers
            firstblock.push_back(0); 
      }
      ///Build Fat
      fat.push_back(fatsize+1); //fat[0] = root
      for(int i = 1; i <= fatsize; i++)
      {
         fat.push_back(0); //fat[1:fatsize] = all zeros
      }
      for(int i = fatsize + 1; i < getnumberofblocks(); i++)
      {
            fat.push_back(i+1); //rest of fat
      }
      fat[fat.size()-1] = 0; //end of fat;
      fssynch();
   }
   else
   {
      istringstream instream;
      instream.str(buffer);
         //read in root
      for(int i = 0; i < rootsize; i++)
      {
         string file;
         int block;
         instream >> file >> block; //input file and block from stream
         filename.push_back(file); //push back filename to filename vector
         firstblock.push_back(block); //push back first block to firstblock vector
      }
      //read in fat
      string ft; //string to hold all blocks of fat
      for(int i = 1; i <= fatsize; i++) //get blocks 1 to fatsize
      {
         string f; //string to hold individual blocks temporarily
         getblock(i, f); //get block i and store it into f
         ft += f; //add f onto ft
      }
      istringstream fatstream;
      fatstream.str(ft);
      for(int i = 0; i < getnumberofblocks(); i++)
      {   
         int n = 0;
         fatstream >> n; // input stream to integer n
         fat.push_back(n); //push back n to fat
      }   
   }//end else
}
int Filesys::fsclose()
{
   return fssynch();
}
int Filesys::fssynch()
{
   //write root to disk
   string rootbuffer;
   ostringstream rootstream;
   for(int i = 0; i < rootsize; i++)
   {
      rootstream << filename[i] << " " << firstblock[i] << " ";
   }
   rootbuffer = rootstream.str();
   vector<string> rootblock = block(rootbuffer, getblocksize()); //pads the end of rootblock
   putblock(0,rootblock[0]); // write rootblock pieces to disk   
   
   //write fat to disk
   string fatbuffer;
   ostringstream fatstream;
   for(int i = 0; i < fat.size(); i++)
   {
      fatstream << fat[i] << " "; // write fat pieces to outstream seperated by space
   }
   fatbuffer = fatstream.str(); // string buffer holds outstream result
   vector<string> fatblocks = block(fatbuffer, getblocksize()); //seperate buffer into blocksize pieces
   for(int i = 0; i < fatblocks.size(); i++)
   {
      putblock(i+1,fatblocks[i]); // write blocksize pieces to disk
   }
}
int Filesys::newfile(string file)
{
   for(int i = 0; i < filename.size(); i++) //check if file exists
   {
      if(filename[i] == file)
      {
         cout << "File already exists" << endl;
         return -1; //file already exists;
      }
   }
   for(int i = 0; i < filename.size(); i++) //check if there is free space ("xxxxx")
   {
      if(filename[i] == "xxxxx")
      {
         filename[i] = file; // replace free space with filename
         firstblock[i] = 0; //firstblock is root
         fssynch(); //sync with disk
         return 1;
      }
   }
   return -1; // no freespace
}
int Filesys::rmfile(string file)
{
   for(int i = 0; i < filename.size(); i++) //check for file
   {
      if(filename[i] == file) //if file exists
      {
         if(firstblock[i] == 0) //if first block is 0, then no blocks attached to the file
         {
            filename[i] = "xxxxx";
            fssynch(); //write to disk
            return 1; //file removed
         }
         else
         {
            cout << "File cannot be deleted because file contains data" << endl;
            return -1; //file contains blocks   
         }
      }
   }
   cout << "File does not exist" << endl;
   return -1; //file does not exist
}
int Filesys::getfirstblock(string file)
{
   for(int i = 0; i < filename.size(); i++)
   {
      if(filename[i] == file)
      {
         return firstblock[i];
      }
   }
   return -1;   
}
int Filesys::addblock(string file, string buffer)
{
   int block = getfirstblock(file);
   
   int allocate;
   if(fat[0] == fat.size()-1)
   {
      cout << "Disk is full" << endl;
      return -1;
   }

   if(block == -1)
   {
      cout << "File does not exist" << endl;
      return 0;
   }   
   else if(block == 0) //file has no blocks, add first block
   {
      allocate = fat[0]; // allocate = 1st free space
      fat[0] = fat[fat[0]]; // 1st free space is replaced with 2nd free space
      fat[allocate] = 0; //old free space is now end of file (0)
      for(int i = 0; i < filename.size(); i++)
      {
         if(filename[i] == file)
         {
            firstblock[i] = allocate; //set first block of the file equal to allocate
         }      
      }
   }
   else
   {   
      allocate = fat[0]; // allocate = 1st free space
      fat[0] = fat[fat[0]]; // 1st free space is replaced with 2nd free space
      fat[allocate] = 0; //old free space is now end of file (0)
      while(fat[block] != 0)
      {
         block = fat[block]; //go through each block of the file until you reach the end of file
      }
      fat[block] = allocate; //that space that had the end of file now points allocate
   }
   fssynch(); //sync file system
   putblock(allocate,buffer); //write the block onto the disk
   return 1; //succcess
}
int Filesys::delblock(string file, int blocknumber)
{
   int block = getfirstblock(file);

   if(block <= 0)
   { //either file doesn't exist, or file contains no blocks
      cout << "No blocks associated with the filename" << endl;
      return 0;
   }
   else if(block == blocknumber)//we're deleting first block of the file
   {
      for(int i = 0; i < filename.size(); i++)
      {
         if(filename[i] == file)
         {
            firstblock[i] = fat[block]; //first block of the file is now the 2nd block
         }
      }
   }
   else //we're deleting some other block
   {   
      while(fat[block] != blocknumber && fat[block] != 0) //go through every block of the file until you hit
      {                         // the block that points to the block you want to delete
         block = fat[block]; 
      }
      if(fat[block] != 0) // fat[blocknumber] = next block after the block
      {
         fat[block] = fat[blocknumber]; //skip the chain 
      }
      else //you hit the end of file
      {
         cout << "Error in deleting block. Block does not belong to the file" << endl;
         return 0;
      }
   }
   
   fat[blocknumber] = fat[0]; //blocknumber now points to 1st freespace
   fat[0] = blocknumber; //1st free space is now the block that was deleted
   fssynch();
   return 1; // success
}
int Filesys::readblock(string file, int blocknumber, string& buffer)
{
   if(checkblock(file,blocknumber) == false)
   {
      return 0;
   }
   getblock(blocknumber,buffer);
   return 1;
}
int Filesys::writeblock(string file, int blocknumber, string buffer)
{
   if(checkblock(file,blocknumber) == false)
   {
      return -1;
   }
   if(buffer.length() > getblocksize())
   {
      cout << "Error. Too large to write to a single block" << endl;
      return -1;
   }
   vector<string> buffers = block(buffer, getblocksize());
   putblock(blocknumber,buffers[0]);
   return 1;
}
int Filesys::nextblock(string file, int blocknumber)
{
   if(checkblock(file,blocknumber) == false)
   {
      return -1;
   }
   return fat[blocknumber];
   
}
bool Filesys::checkblock(string file, int blocknumber)
{
   int block = getfirstblock(file); //return the first block of file
   if(getfirstblock(file) == blocknumber)
   {
      return true;   
   }
   while(fat[block] != blocknumber && fat[block] != 0) //go through every block of the file until you hit
   {                         // the block that points to the block of interest
      block = fat[block]; 
   }
   if(fat[block] != 0) // fat[blocknumber] = next block after the block
   {
      return true;
   }
   else //you hit the end of file
   {
      cout << "Blocknumber does not belong to the file" << endl;
      return false;
   }
}
