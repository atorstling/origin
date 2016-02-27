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
print check("ll", ["'ls' found in PATH as '/bin/ls'",
                   "'/bin/ls' is an executable"]);
# built-in 
print check("type", ["'type' is a shell builtin"]);
# executable in path 
print check("uname", ["'/bin/uname' is an executable"]);
# symlink in path
print check("sh", 
            ["'sh' found in PATH as '/bin/sh'",
             "'/bin/sh' is a symlink to '/bin/dash'", 
            "'/bin/dash' is an executable"]);

print "OK"
