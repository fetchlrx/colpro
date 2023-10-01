#include "PageTable.h"

/****************************************************************
 * Name(s): Jacob LeReaux
 * File: PageTable.cpp
 ****************************************************************/

PageTable::PageTable()
{
}
void PageTable::set_size(int pages)
{
	table = vector<int>(pages,0);
}
void PageTable::set_frame(int index, int frame)
{
	frame <<= 2;
	table[index] &= 3;  //reset any previous frame
    table[index] |= (frame); // or with frame number
	set_valid(index, 1);
}//set_frame 

void PageTable::set_modify_bit(int index, int value)
{
	table[index] &= 126;
	table[index] |= (value);
}//set_mod
/*
* set_valid()
*	- tells  the virtual machine that the page is already in memory
*/
void PageTable::set_valid(int index, int value)
{
    table[index] &= 125; //1111101
    table[index] |= (value << 1);
}//set_valid

int PageTable::get_frame(int index)
{
    int value = table[index] & f_mask; //0111 1100
    value >>= 2;
    return value;
}//get_frame

bool PageTable::get_mod(int index)
{
    int value = table[index] & m_mask; //0000 0001
	
	if( value == 1 ){	
		return true;
	}
    return false;
}//get_mod

bool PageTable::get_valid(int index)
{
    int value = table[index] & v_mask; //0000 0010
    value >>= 1;
	if( value == 1 ){	
		return true;
	}
    return false;
}//get_valid
