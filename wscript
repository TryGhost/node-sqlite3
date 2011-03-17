import Options
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
  if not conf.check(lib="sqlite3", libpath=['/usr/local/lib', '/opt/local/lib'], uselib_store="SQLITE3"):
    conf.fatal('Missing sqlite3');

def build(bld):
  obj = bld.new_task_gen("cxx", "shlib", "node_addon")
  obj.cxxflags = ["-g", "-D_FILE_OFFSET_BITS=64", "-D_LARGEFILE_SOURCE",
                  "-DSQLITE_ENABLE_RTREE=1", "-Wall"]
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