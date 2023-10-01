#ifndef PAGE_TABLE_H
#define PAGE_TABLE_H

#include "defines.h"
#include <vector>
#include <fstream>

/****************************************************************
 * Name(s): Jacob LeReaux, Michael Kang 
 * File: PageTable.h
 ****************************************************************/

using namespace std;

class PageTable
{
public:
    PageTable();
    //getters
    bool get_mod(int index); 
    bool get_valid(int index); 
    int get_frame(int index);
    
    //setters
    void set_frame(int index, int frame);
    void set_modify_bit(int index, int value);
    void set_valid(int index, int value);
    void set_size(int pages);
    //void fill_table(string file);

	vector<int> table; 
private:
    static const int f_mask = 124; //0111 1100
    static const int v_mask = 2; //0000 0010 valid = 1, invalid = 0
    static const int m_mask = 1; //0000 0001 modify bit
    friend class PCB;
};

#endif
