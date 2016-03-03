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
print check("ll", 0, ["'ll' is an alias for 'ls' in shell '/bin/bash': 'ls -alF'",
                      "'ls' is an alias for 'ls' in shell '/bin/bash': 'ls --color=auto'",
                      "'ls' found in PATH as '/bin/ls'",
                      "'/bin/ls' is an executable",
                      "target reached"]);
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
print check("java", 0,  
            ["'java' found in PATH as '/usr/bin/java'",
             "'/usr/bin/java' is a symlink to '/etc/alternatives/java'", 
            "'/etc/alternatives/java' is a symlink to '/usr/lib/jvm/java-8-openjdk-amd64/jre/bin/java",
            "'/usr/lib/jvm/java-8-openjdk-amd64/jre/bin/java' is an executable",
            "target reached"]);
print check("sant", 0,
           ["'sant' is a symlink to '/bin/../bin/true'",
            "'/bin/../bin/true' is an executable",
            "'/bin/../bin/true' has canonical pathname '/bin/true'"]);

print "OK"
