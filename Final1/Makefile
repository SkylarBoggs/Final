CC = g++
CFLAGS = -Wall -g

all: build archive

build: final.o 
	$(CC) $(CFLAGS) $^ -o count

final.o: final.cc
	$(CC) $(CFLAGS) -c $<

make doc: final.cc
	@doxygen ./Doxyfile

make check:
	

clean: 
	-rm -f final.o count


	


