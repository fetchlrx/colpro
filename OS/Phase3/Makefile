all : os

BUILD_DIR = .

CC = g++

OBJECTS = VirtualMachine.o \
			Assembler.o \
			PageTable.o \
			PCB.o

os : $(OBJECTS)
	$(CC) -o OS os.cpp $(OBJECTS)
	
*.o :
	$(CC) $(CFLAGS) -c $<

clean:
	rm *.o
	rm OS