CC=clang
CFLAGS=-Wall -pedantic-errors -I.
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
	rm -f $(ODIR)/*
