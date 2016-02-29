CC=clang
#CFLAGS=-std=c99 -pedantic-errors -Wall -Wextra -Wshadow -Wpointer-arith \
 -Wcast-qual -Wstrict-prototypes -Wmissing-prototypes -I.
# -Wno-padded 
CFLAGS=-std=c11 -O0 -g -D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=700 -Weverything -Werror -Wno-format-nonliteral
#CFLAGS+=--analyze -Xanalyzer -analyzer-output=text
OUT=target/trejs
ODIR=target
SDIR=src
_OBJS=trejs.o
OBJS=$(patsubst %,$(ODIR)/%,$(_OBJS))

$(ODIR)/%.o: $(SDIR)/%.c
	$(CC) -c -o $@ $< $(CFLAGS)

$(OUT): $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -rf $(ODIR)/*

make:
	scan-build make

test: $(OUT)
	./tests.py
