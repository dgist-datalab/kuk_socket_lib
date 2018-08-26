export CC=gcc


TARGET :=kuk_sock.o

all: $(TARGET)

.c.o:
	$(CC) -c $< -o $@

clean:
	$(RM) *.o
