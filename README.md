# Track
Track down the origin of a command - `which` on steroids

# Examples
```bash
alext@smith:~/projects/track$ ./track ll
'll' is an alias for 'ls' in shell '/bin/bash': 'ls -alF'
'ls' is an alias for 'ls' in shell '/bin/bash': 'ls --color=auto'
'ls' found in PATH as '/bin/ls'
'/bin/ls' is an executable
target reached
alext@smith:~/projects/track$ ./track type
'type' is built into shell '/bin/bash'
target reached
alext@smith:~/projects/track$ ./track /bin/uname
'/bin/uname' is an executable
target reached
alext@smith:~/projects/track$ ./track uname
'uname' found in PATH as '/bin/uname'
'/bin/uname' is an executable
target reached
alext@smith:~/projects/track$ ./track sh
'sh' found in PATH as '/bin/sh'
'/bin/sh' is a symlink to '/bin/dash'
'/bin/dash' is an executable
target reached
alext@smith:~/projects/track$ ./track java
'java' found in PATH as '/usr/bin/java'
'/usr/bin/java' is a symlink to '/etc/alternatives/java'
'/etc/alternatives/java' is a symlink to '/usr/lib/jvm/java-8-openjdk-amd64/jre/bin/java'
'/usr/lib/jvm/java-8-openjdk-amd64/jre/bin/java' is an executable
target reached
alext@smith:~/projects/track$ ./track nonexistent
no match
alext@smith:~/projects/track$
```
#Limitations
Only works with bash ATM. Will not read aliases from the current shell, but from a sub-shell which is launched.

#Building
Tested on latest Ubuntu and OS X. Just run `make`. Requires `clang` with `c11`, `POSIX.1-2008` and `X/Open 7`
