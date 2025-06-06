# 'make DEBUG=0' disables debug mode
DEBUG ?= 1
ifeq ($(DEBUG), 1)
    CFLAGS+=-O0 -g
else
    CFLAGS+=-O3 -Wno-disabled-macro-expansion
endif

ifeq ($(shell uname), Darwin)
    PLATFORM=macos
else ifeq ($(shell uname -o), Msys)
    PLATFORM=msys
else ifeq ($(shell uname -o), GNU/Linux)
    PLATFORM=linux
endif

# Comments about flags on Darwin vs Linux: 
# https://lwn.net/Articles/590381/
ifeq ($(PLATFORM), linux)
	CC=clang
	CFLAGS=-std=c11 -Werror -Wno-format-nonliteral -Weverything -Wno-declaration-after-statement -Wno-unsafe-buffer-usage -Wwrite-strings -D_GNU_SOURCE -D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=700
else ifeq ($(PLATFORM), msys)
	CC=gcc
	CFLAGS=-std=gnu11 -Werror -Wno-format-nonliteral -Wwrite-strings -Wall -Wextra -Wshadow -Wpedantic -Wno-unknown-pragmas
else
	#Including Darwin
	CC=clang
	# The poison system directories flag is a workaround for https://github.com/dotnet/runtime/issues/41095
	CFLAGS=-std=c11 -Werror -Wno-format-nonliteral -Weverything -Wno-declaration-after-statement -Wno-poison-system-directories
endif
# gperftools
# 'make PROFILE=1' enables profiler mode
PROFILE ?= 0
ifeq ($(PROFILE), 1)
    ifneq ($(PLATFORM), msys)
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
	CPUPROFILE=$(PROFOUT) CPUPROFILE_REALTIME=1 $(OUT) ll

profile: $(PROFOUT)

view-profile: $(PROFOUT)
	google-pprof $(OUT) $(PROFOUT)
