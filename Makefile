#CC=clang
#CFLAGS=-Weverything -Werror -Wno-format-nonliteral
CC=gcc
CFLAGS=-Werror -Wno-format-nonliteral
# 'make DEBUG=0' disables debug mode
DEBUG ?= 1
ifeq ($(DEBUG), 1)
    CFLAGS+=-O0 -g
else
    CFLAGS+=-O3 -Wno-disabled-macro-expansion
endif
UNAME := $(shell uname -o)
# Comments about flags on Darwin vs Linux: 
# https://lwn.net/Articles/590381/
ifeq ($(UNAME), GNU/Linux)
	CFLAGS+=-std=c11 -D_GNU_SOURCE -D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=700 
else ifeq ($(UNAME), Msys)
	CFLAGS+=-std=gnu11
else
	#Including Darwin
	CFLAGS+=-std=c11
endif
# gperftools
# 'make PROFILE=0' disables profiler mode
PROFILE ?= 1
ifeq ($PROFILE), 1)
		LFLAGS=-lprofiler
else
		LFLAGS=
endif
ODIR=target
OUT=$(ODIR)/origin
SDIR=src
_OBJS=origin.o
OBJS=$(patsubst %,$(ODIR)/%,$(_OBJS))
PROFOUT=$(ODIR)/prof.out

compile: $(OUT)

all: analyze check

$(ODIR):
	mkdir $(ODIR)

$(OUT): $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS) $(LFLAGS) 

$(ODIR)/%.o: $(SDIR)/%.c $(ODIR)
	$(CC) -c -o $@ $< $(CFLAGS)

clean:
	rm -rf $(ODIR)

analyze: 
	scan-build --status-bugs --use-cc=clang make clean $(OUT)

check: $(OUT)
	./run_tests.sh

$(PROFOUT): $(OUT)
	CPUPROFILE=$(PROFOUT) CPUPROFILE_REALTIME=1 $(OUT) ll

profile: $(PROFOUT)

view-profile: $(PROFOUT)
	google-pprof $(OUT) $(PROFOUT)
