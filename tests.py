#!/usr/bin/env python
import subprocess
import os
import sys
from subprocess import Popen

bash_path="/bin/bash"
ls_path="/bin/ls"
uname_path="/bin/uname"
sh_path="/bin/sh"
sh_resolve_path="/bin/sh"
sant_link_name="/bin/sant"
true_path="/bin/true"

def check(args, expected_code, expected_texts):
  child_env = os.environ.copy()
  # Since we move bash to /bin/bash
  child_env["SHELL"] = "/bin/bash"
  child_env["PATH"] += ":/bin"
  child_env["HOME"] = "/home/alexa"
  cmd = "chroot target/jail_root origin " + args
  p = Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, env=child_env)
  code = p.wait()
  out, err = p.communicate()
  all = out + err
  all_decoded = all.decode("utf-8")
  if code != expected_code:
    raise Exception("command '%s' exited with status %s, expected %s. Output:\n%s\n" % ( cmd, code, expected_code, all_decoded))
  for e in expected_texts:
    if not e in all_decoded:
      raise Exception("expected \n===\n%s\n===\nin command output: \n===\n%s\n===\n" % (e, all_decoded))
  return (cmd, out, err) 

# no arguments
print(check("", 2, ["origin: Usage: origin [-v] command"]));
# non-existent
print(check("miss", 1, ["no match"]));
# multi-level alias
print(check("ll", 0, ["'ll' is an alias for 'ls' in shell '%s': 'ls -alF'" % bash_path,
                      "'ls' is an alias for 'ls' in shell '%s': 'ls --color=auto'" % bash_path,
                      "'ls' found in PATH as '%s'" % ls_path,
                      "'%s' is an executable" % ls_path]));
# built-in 
print(check("type", 0, ["'type' is built into shell '%s'" % bash_path]));
# file
print(check(uname_path, 0, ["'%s' is an executable" % uname_path]));
# executable in path 
print(check("uname", 0, ["'%s' is an executable" % uname_path]));
# symlink in path
print(check("sh", 0,
            ["'sh' found in PATH as '%s'" % sh_path,
            "'%s' is an executable" % sh_resolve_path]));
# multi-step
print(check(sant_link_name, 0,
           ["'%s' is a symlink to" % (sant_link_name)
            ]));
print(check("/", 0,
            ["'/' is a regular file"]));
print(check(".", 0, ["'.' is built into shell '%s'" % bash_path]));
print(check("afunction", 0, ["'afunction' is a function in shell '%s':" % bash_path,
                             "afunction ()",
                             "{",
                             "    echo \"I am a function\"",
                             "}"]));

print("OK")
