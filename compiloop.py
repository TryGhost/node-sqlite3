#!/usr/bin/python
# Wait for changes for files specified on the command line, or here in the file
# Then compile and run test.  And do this forever.

import os, sys, time

#filenames = sys.argv[1:]
filenames = ["sqlite3_bindings.cc", "wscript", "sqlite.js", "test.js"]

def handler():
  os.system("clear; rm -f test.db")
  os.system("node-waf build && node test.js && sleep 1 && sqlite3 test.db .dump");

mtime = []
while True:
  m = [os.stat(filename).st_mtime for filename in filenames]
  if mtime != m:
    handler()
    mtime = m

  time.sleep(1)

