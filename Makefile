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
	CC=clang
	CFLAGS=-std=c11 -Werror -Wno-format-nonliteral -Weverything -D_GNU_SOURCE -D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=700 
else ifeq ($(UNAME), Msys)
	CC=gcc
	CFLAGS=-std=gnu11 -Werror -Wno-format-nonliteral
else
	#Including Darwin
	CC=clang
	CFLAGS=-std=c11 -Werror -Wno-format-nonliteral -Weverything
endif
# gperftools
# 'make PROFILE=0' disables profiler mode
PROFILE ?= 1
ifeq ($(PROFILE), 1)
ifneq ($(UNAME), Msys)
		LFLAGS=-lprofiler
ifeq ($(CC), gcc)
		CFLAGS+=-Wl,--no-as-needed
endif
else
		LFLAGS=
endif
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
	SHELL=$(SHELL) CPUPROFILE=$(PROFOUT) CPUPROFILE_REALTIME=1 $(OUT) ll

profile: $(PROFOUT)

view-profile: $(PROFOUT)
	google-pprof $(OUT) $(PROFOUT)
