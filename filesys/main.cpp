#include "sdisk.h"
#include "filesys.h"
	
vector<string> block(string buffer, int blocknumber);

int main()
{
  Sdisk disk1("disk1",256,128);
  Filesys fsys("disk1",256,128);
  fsys.newfile("file1");
  fsys.newfile("file2");

  string bfile1;
  string bfile2;

  for (int i=1; i<=1024; i++)
  {
    bfile1+="1";
  }

  vector<string> blocks=block(bfile1,128); 

  int blocknumber=0;

  for (int i=0; i< blocks.size(); i++)
  {
    blocknumber=fsys.addblock("file1",blocks[i]);
  }

  fsys.delblock("file1",fsys.getfirstblock("file1"));

  for (int i=1; i<=2048; i++)
     {
       bfile2+="2";
     }

  blocks=block(bfile2,128); 

  for (int i=0; i< blocks.size(); i++)
  {
    blocknumber=fsys.addblock("file2",blocks[i]);
  }

  fsys.delblock("file2",blocknumber);

  string buffer = "88888888888888888888888888888888888888888888";
  blocks = block(buffer,128);
  
  fsys.writeblock("file2",36,blocks[0]);

  buffer.clear();
  fsys.readblock("file2",36, buffer);
	
  cout << buffer << endl;

  cout << "file 2 next block = " << fsys.nextblock("file2",14);
   

}
