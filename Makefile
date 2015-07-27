CC     = gcc
CFLAGS = -D_REENTRANT -g -Wall -pedantic -Isrc -std=gnu99
LDLIBS = -lpthread

all: src/9p.o src/threadpool.o src/server.o src/rmessage.o src/rfunctions.o src/fid.o
	$(CC) $(CFLAGS) -o server src/server.o src/threadpool.o src/9p.o src/rmessage.o src/rfunctions.o src/fid.o $(LDLIBS)

src/threadpool.o: src/threadpool.c src/threadpool.h
	$(CC) $(CFLAGS) -o $@ -c src/threadpool.c $(LDLIBS)

src/server.o: src/server.c src/9p.h src/threadpool.h src/rmessage.h
	$(CC) $(CFLAGS) -o $@ -c src/server.c
	
src/rmessage.o: src/rmessage.c src/rfunctions.h src/9p.h src/fid.h
	$(CC) $(CFLAGS) -o $@ -c src/rmessage.c
	
src/rfunctions.o: src/rfunctions.c
	$(CC) $(CFLAGS) -o $@ -c src/rfunctions.c
		
src/9p.o: src/9p.c src/9p.h
	$(CC) $(CFLAGS) -o $@ -c src/9p.c 

src/fid.o: src/fid.h src/fid.c
	$(CC) $(CFLAGS) -o $@ -c src/fid.c
	
clean:
	rm -f $(TARGETS) *~ */*~ */*.o

