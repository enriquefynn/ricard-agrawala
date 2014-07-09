MPICC=mpic++
MPICCFLAGS=-std=c++11

all: ricart-agrawala.cpp
	${MPICC} ${MPICCFLAGS} -o ricart-agrawala ricart-agrawala.cpp
clean:
	rm -rf ricart-agrawala *.o *~ poem.txt position.txt
