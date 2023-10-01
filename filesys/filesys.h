//Jacob LeReaux - 004961563
// Lab 4 - CSE461: Tuesday
//05-03-16

#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>

using namespace std;

class Filesys: public Sdisk
{
    public:
    	Filesys(string diskname, int numberofblocks, int blocksize);
    	int fsclose(); //closes the file system
    	int fssynch(); //writes the current fat and root onto the disk 
    	int newfile(string file);
    	int rmfile(string file);
    	int getfirstblock(string file);
    	int addblock(string file, string block);
    	int delblock(string file, int blocknumber);
	int readblock(string file, int blocknumber, string& buffer);
    	int writeblock(string file, int blocknumber, string buffer);
    	int nextblock(string file, int blocknumber);
    private:
	bool checkblock(string file, int blocknumber);
	int rootsize;           // maximum number of entries in ROOT
	int fatsize;            // number of blocks occupied by FAT
	vector<string> filename;   // filenames in ROOT
	vector<int> firstblock; // firstblocks in ROOT
	vector<int> fat;             // FAT
};
