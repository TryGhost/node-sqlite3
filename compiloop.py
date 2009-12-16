#!/usr/bin/python
# Wait for changes to relevant files, then compile and run test. And
# do this forever.

import os, sys, time

filenames = ["sqlite3_bindings.cc", "wscript", "sqlite.js", "test.js"]

mtime = []
while True:
  m = [os.stat(filename).st_mtime for filename in filenames]
  if mtime != m:
    os.system("clear; rm -f test.db")
    os.system("node-waf build && node test.js && sleep 1 && sqlite3 test.db .dump");
    mtime = m

  time.sleep(1)

