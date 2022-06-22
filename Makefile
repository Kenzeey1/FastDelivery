default: clean all

cassini : timing-text-io
	$(CC) -Wall ./src/cassini.c -I ./include timing-text-io.o -o cassini

saturnd : timing-text-io
	$(CC) -Wall ./src/saturnd.c -I ./include timing-text-io.o -o saturnd

timing-text-io :
	$(CC) -c -Wall ./src/timing-text-io.c -I ./include -o timing-text-io.o


clean:
	rm -f *.o
	rm -f cassini
	rm -f saturnd

all: cassini saturnd
