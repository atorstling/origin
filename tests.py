#!/usr/bin/env python
import subprocess;

def check(cmd, expected):
  out = run(cmd)
  for e in expected:
    if not e in out: 
      raise Exception("expected '%s' in '%s'" % (e, out))
  return out

def run(cmd):
  return subprocess.check_output("./run.sh " + cmd, shell=True, stderr=subprocess.STDOUT);


# non-existent
print check("miss", ["no match"]);
# multi-level alias
print check("ll", ["'ls' found in path as executable '/bin/ls'"]);
# executable without indirection
print check("uname", ["executable '/bin/uname'"]);
# built-in 
print check("type", ["'type' is a shell builtin"]);
# symlink
print check("sh", 
            ["'sh' found in path as symlink '/bin/sh' to '/bin/dash'",
            "'/bin/dash' is an executable"]);

print "OK"
