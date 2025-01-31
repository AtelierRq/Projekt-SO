CC = gcc
CFLAGS = -Wall -g
LDLIBS = -lpthread

all: initializer worker fan manager

initializer: initializer.c utils.c stadium.h
	$(CC) $(CFLAGS) initializer.c utils.c -o initializer

worker: worker.c utils.c stadium.h
	$(CC) $(CFLAGS) worker.c utils.c -o worker $(LDLIBS)

fan: fan.c utils.c stadium.h
	$(CC) $(CFLAGS) fan.c utils.c -o fan

manager: manager.c utils.c stadium.h
	$(CC) $(CFLAGS) manager.c utils.c -o manager

clean:
	rm -f initializer worker fan manager
