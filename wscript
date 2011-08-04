import os
import Options
from Configure import ConfigurationError
from os.path import exists
from shutil import copy2 as copy

TARGET = 'sqlite3_bindings'
TARGET_FILE = '%s.node' % TARGET
built = 'build/default/%s' % TARGET_FILE
dest = 'lib/%s' % TARGET_FILE

def set_options(opt):
  opt.tool_options("compiler_cxx")

def configure(conf):
  conf.check_tool("compiler_cxx")
  conf.check_tool("node_addon")
  
  linkflags = []
  if os.environ.has_key('LINKFLAGS'):
      linkflags.extend(os.environ['LINKFLAGS'].split(' '))
  conf.env.append_value("LINKFLAGS", linkflags)

  try:
    conf.check_cfg(package="sqlite3", args='--libs --cflags',
                   uselib_store="SQLITE3", mandatory=True)
  except ConfigurationError:
    conf.check(lib="sqlite3", libpath=['/usr/local/lib', '/opt/local/lib'],
               uselib_store="SQLITE3", mandatory=True)

def build(bld):
  obj = bld.new_task_gen("cxx", "shlib", "node_addon")
  obj.cxxflags = ["-g", "-D_FILE_OFFSET_BITS=64", "-D_LARGEFILE_SOURCE",
                  "-DSQLITE_ENABLE_RTREE=1", "-pthread", "-Wall"]
  obj.target = TARGET
  obj.source = "src/sqlite3.cc src/database.cc src/statement.cc"
  obj.uselib = "SQLITE3"

def shutdown():
  if Options.commands['clean']:
      if exists(TARGET_FILE):
        unlink(TARGET_FILE)
  else:
    if exists(built):
      copy(built, dest)
