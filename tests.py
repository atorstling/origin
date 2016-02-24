#!/usr/bin/env python
import subprocess;

def check(cmd, expected):
  out = run(cmd)
  if not expected in out: 
    raise Exception("expected '%s' in '%s'" % (expected, out))
  return out

def run(cmd):
  return subprocess.check_output("./run.sh " + cmd, shell=True, stderr=subprocess.STDOUT);


# non-existent
print check("miss", "no match");
# multi-level alias
print check("ll", "already searched for ls, aborting");
# executable without indirection
print check("uname", "executable /bin/uname");
# built-in 
print check("type", "'type' is a shell builtin");

print "OK"
