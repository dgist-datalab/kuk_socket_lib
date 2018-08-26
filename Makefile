#export CC=gcc


TARGET :=kuk_sock.o

all: $(TARGET)

.c.o:
	$(CC) -c $< -g -o $@
test: kuk_sock.c
	$(CC) -o $@ kuk_sock.c
	

clean:
	$(RM) *.o
	$(RM) test
