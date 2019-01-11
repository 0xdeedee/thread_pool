LIBTARGET=libthreadpool.so

CC=gcc
DEPS=
CFLAGS= -fPIC -Wall -Wextra -O3 -D__DEBUG_ENABLED__
LDFLAGS= -shared
OBJ:=$(patsubst %.c,%.o,$(wildcard *.c))

LIBS= -lpthread

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(LIBTARGET): $(OBJ) *.h
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

all: clean $(LIBTARGET)

clean:
	rm -rf *~ $(LIBTARGET) $(OBJ) *.txt .*.swp

