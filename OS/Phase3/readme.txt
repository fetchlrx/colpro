 
 * Description: This Code is an implementation of a low level, 16-bit Operating System.
 *    This OS has multi-programming ability, as well as paging ability. During initialization,
 *    ths OS will take a list of processes listed in a program file. It will command the VM to 
 *    one-by-one load the first page of the first five processes into memory. We do this by 
 *   a process written in simplified Assembly and assemble each written instruction into a 
 *   respective integer, that represents the entire instruction. Once the programs are loaded
 *   into memory, we can begin by adding all processes to the Ready Queue and to begin
 *   execution. The VM will be responsible for keeping track of the time spent in the VM and
 *   giving control back to the OS once a program reaches the specified timeslice. The OS 
 *   will also gain control whenever an IO request is made, or if a page fault occurs.
 *   The OS has an inverted page table to keep track of which process owns each frame, as
 *   well as which page. The OS will be responsible for context switching accordingly and 
 *    to make sure each program is in its respective queue (Ready or Wait), at the time of a 
 *   context switch. During the execution of this OS, the addresses stored inside of a PCB
 *   will be the logical addresses, while, when inside of the VM, will be the physical
 *   addresses. The OS is also responsible for writing any data to the hard disk that a 
 *   program modifies. During execution, the OS will be calculating system information and 
 *   write the information to each program's output file at the end of execution. Once
 *    a program successfully halts, it will load a new process into memory as well as
 *   create the program's PCB. If stack is written during the execution of a program, the OS
 *    will need to update the inverted page table accordingly to specify which frames are used
 *   by stack. Currently, the OS will write stack data to a .st file during context switches.
 * As well as the .st file, each program will have one .s, .o, .out, and one .in file. .s 
 *   files contain Assembly instructions. .o files are the Assembled integer instructions. 
 *   .out files store any output from a program. Lastly, .in files will store any input 
 *   needed by a program. The OS will context switch using a round-robin implementation, 
 *   while all processes currently have the same priority, meaning there is no preemption. 