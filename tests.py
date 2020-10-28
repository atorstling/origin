#!/usr/bin/env python
import subprocess;
import os;
from subprocess import Popen;

exename = "target/origin.exe"


def check(args, expected_code, expected_texts):
  cmd = exename + " " + args
  p = Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE);
  code = p.wait();
  out, err = p.communicate();
  all = out + err
  all_decoded = all.decode("utf-8");
  if code != expected_code:
    raise Exception("command '%s' exited with status %s, expected %s. Output:\n%s\n" % ( cmd, code, expected_code, all))
  for e in expected_texts:
    if not e in all_decoded:
      raise Exception("expected \n===\n%s\n===\nin command output: \n===\n%s\n===\n" % (e, all))
  return (cmd, out, err) 

# no arguments
print(check("", 2, ["target/origin: Usage: target/origin [-v] command"]));
# non-existent
print(check("miss", 1, ["no match"]));
# multi-level alias
print(check("ll", 0, ["'ll' is an alias for 'ls' in shell '/usr/bin/bash': 'ls -alF'",
                      "'ls' is an alias for 'ls' in shell '/usr/bin/bash': 'ls --color=auto'",
                      "'ls' found in PATH as '/usr/bin/ls'",
                      "'/usr/bin/ls' is an executable"]));
# built-in 
print(check("type", 0, ["'type' is built into shell '/usr/bin/bash'"]));
# file
print(check("/usr/bin/uname", 0, ["'/usr/bin/uname' is an executable"]));
# executable in path 
print(check("uname", 0, ["'/usr/bin/uname' is an executable"]));
# symlink in path
print(check("sh", 0,
            ["'sh' found in PATH as '/usr/bin/sh'",
            "'/usr/bin/sh' is an executable"]));
# multi-step
print(check("sant", 0,
           ["'sant' is a symlink to '/usr/bin/true.exe'"
            ]));
print(check("origin", 0,
            ["'origin' is a symlink to './target/origin'",
"'./target/origin' is an executable",
"'./target/origin' has canonical pathname '" + os.getcwd() + "/target/origin'"]));
print(check("/", 0,
            ["'/' is a regular file"]));
print(check(".", 0, ["'.' is built into shell '/usr/bin/bash'"]));
print(check("afunction", 0, ["'afunction' is a function in shell '/usr/bin/bash':",
                             "afunction ()",
                             "{",
                             "    echo \"I am a function\"",
                             "}"]));

print("OK")
