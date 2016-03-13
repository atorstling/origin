#!/usr/bin/env python
import subprocess;
import os;
from subprocess import Popen;


def check(args, expected_code, expected_texts):
  cmd = "./vrun.sh " + args
  p = Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE);
  code = p.wait();
  out, err = p.communicate();
  all = out + err
  if code != expected_code:
    raise Exception("command '%s' exited with status %s, expected %s. Output:\n%s\n" % ( cmd, code, expected_code, all))
  for e in expected_texts:
    if not e in all: 
      raise Exception("expected \n===\n%s\n===\nin command output: \n===\n%s\n===\n" % (e, all))
  return (out, err) 

# no arguments
print check("", 2, ["origin: missing command name"])
# non-existent
print check("miss", 1, ["no match"]);
# multi-level alias
print check("ll", 0, ["'ll' is an alias for 'ls' in shell '/bin/bash': 'ls -alF'",
                      "'ls' is an alias for 'ls' in shell '/bin/bash': 'ls --color=auto'",
                      "'ls' found in PATH as '/bin/ls'",
                      "'/bin/ls' is an executable"]);
# built-in 
print check("type", 0, ["'type' is built into shell '/bin/bash'"]);
# file
print check("/bin/uname", 0, ["'/bin/uname' is an executable"]);
# executable in path 
print check("uname", 0, ["'/bin/uname' is an executable"]);
# symlink in path
print check("sh", 0,  
            ["'sh' found in PATH as '/bin/sh'",
             "'/bin/sh' is a symlink to '/bin/dash'", 
            "'/bin/dash' is an executable"]);
# multi-step
print check("write", 0,  
            ["'write' found in PATH as '/usr/bin/write'",
             "'/usr/bin/write' is a symlink to '/etc/alternatives/write'", 
            "'/etc/alternatives/write' is a symlink to '/usr/bin/bsd-write",
            "'/usr/bin/bsd-write' is an executable" ]);
print check("sant", 0,
           ["'sant' is a symlink to '/bin/../bin/true'",
            "'/bin/../bin/true' is an executable",
            "'/bin/../bin/true' has canonical pathname '/bin/true'"]);
print check("origin", 0,
            ["'origin' is a symlink to './target/origin'",
"'./target/origin' is an executable",
"'./target/origin' has canonical pathname '" + os.getcwd() + "/target/origin'"]
)
print check("/", 0, 
            ["'/' is a regular file"]);

print "OK"
