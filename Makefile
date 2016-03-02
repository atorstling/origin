CC=clang
#CFLAGS=-std=c99 -pedantic-errors -Wall -Wextra -Wshadow -Wpointer-arith \
 -Wcast-qual -Wstrict-prototypes -Wmissing-prototypes -I.
CFLAGS=-std=c11 -O0 -g -D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=700 -Weverything -Werror -Wno-format-nonliteral
OUT=target/track
ODIR=target
SDIR=src
_OBJS=track.o
OBJS=$(patsubst %,$(ODIR)/%,$(_OBJS))

all: target $(OUT)

target:
	mkdir target

$(OUT): $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS)

$(ODIR)/%.o: $(SDIR)/%.c
	$(CC) -c -o $@ $< $(CFLAGS)

clean:
	rm -rf $(ODIR)/*

analyze: 
	scan-build --use-cc=clang make clean compile

check: $(OUT)
	./tests.py
