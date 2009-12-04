#!/usr/bin/python
# Wait for changes for files specified on the command line, or here in the file
# Then compile and run test.  And do this forever.

import os, sys, time

#filenames = sys.argv[1:]
filenames = ["sqlite3_bindings.cc", "wscript", "sqlite3.js", "test.js"]
mdname = "README"

def handler():
  os.system("clear; rm -f test.db")
  os.system("node-waf build && node test.js && sqlite3 test.db .dump");

mtime = []
mdtime = None
while True:
  m = [os.stat(filename).st_mtime for filename in filenames]
  if mtime != m:
    handler()
    mtime = m

  m = os.stat(mdname).st_mtime
  if mdtime != m:
    os.system("Markdown.pl < %s > %s.html" % (mdname, mdname))
    mdtime = m

  time.sleep(1)

