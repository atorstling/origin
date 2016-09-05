# Track
Track down the origin of a command - a recursive `which` on steroids

# Examples
```bash
alext@smith:~/projects/origin$ ./origin ll
'll' is an alias for 'ls' in shell '/bin/bash': 'ls -alF'
'ls' is an alias for 'ls' in shell '/bin/bash': 'ls --color=auto'
'ls' found in PATH as '/bin/ls'
'/bin/ls' is an executable
target reached
alext@smith:~/projects/origin$ ./origin type
'type' is built into shell '/bin/bash'
target reached
alext@smith:~/projects/origin$ ./origin /bin/uname
'/bin/uname' is an executable
target reached
alext@smith:~/projects/origin$ ./origin uname
'uname' found in PATH as '/bin/uname'
'/bin/uname' is an executable
target reached
alext@smith:~/projects/origin$ ./origin sh
'sh' found in PATH as '/bin/sh'
'/bin/sh' is a symlink to '/bin/dash'
'/bin/dash' is an executable
target reached
alext@smith:~/projects/origin$ ./origin java
'java' found in PATH as '/usr/bin/java'
'/usr/bin/java' is a symlink to '/etc/alternatives/java'
'/etc/alternatives/java' is a symlink to '/usr/lib/jvm/java-8-openjdk-amd64/jre/bin/java'
'/usr/lib/jvm/java-8-openjdk-amd64/jre/bin/java' is an executable
target reached
alext@smith:~/projects/origin$ ./origin nonexistent
no match
alext@smith:~/projects/origin$
```
#Limitations
Only works with bash (contributions for other shells welcome). Will not read aliases from the current shell, but from a spawned sub-shell. This means that aliases and other commands configured for login shells (in `~/.bashrc` or otherwise) will be shown. Aliases only defined locally in the calling shell won't be shown.

#Building
Tested on latest Ubuntu and OS X. Just run `make`. Requires `clang` (should work with gcc though), `c11`, `POSIX.1-2008`, `X/Open 7`. Links google performance tools, but this can easily be disabled.

##With Docker
The easiest way of building is running in docker:
```
./docker-build && ./docker-run make check
```
