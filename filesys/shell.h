

#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>

using namespace std;

class Shell: public Filesys
{
   public:
      Shell(string diskname, int blocksize, int numberofblocks);
      int dir();// lists all files
      int add(string file);// add a new file using input from the keyboard
      int del(string file);// deletes the file
      int type(string file);//lists the contents of file
      int copy(string file1, string file2);//copies file1 to file2
   private:
      string diskname;
      int numberofblocks;
      int blocksize;
};

