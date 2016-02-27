#!/usr/bin/env python
import subprocess;
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
      raise Exception("expected '%s' in '%s'" % (e, all))
  return (out, err) 

# non-existent
print check("miss", 1, ["no match"]);
# multi-level alias
print check("ll", 0, ["'ls' found in PATH as '/bin/ls'",
                   "'/bin/ls' is an executable"]);
# built-in 
print check("type", 0, ["'type' is a shell builtin"]);
# file
print check("/bin/uname", 0, ["'/bin/uname' is an executable"]);
# executable in path 
print check("uname", 0, ["'/bin/uname' is an executable"]);
# symlink in path
print check("sh", 0,  
            ["'sh' found in PATH as '/bin/sh'",
             "'/bin/sh' is a symlink to '/bin/dash'", 
            "'/bin/dash' is an executable"]);

print "OK"
