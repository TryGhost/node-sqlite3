# vim: ft=javascript
import Options
from os import unlink, symlink, system
from os.path import exists, abspath

srcdir = "."
blddir = "build"
VERSION = "0.0.1"

def set_options(opt):
  opt.tool_options("compiler_cxx")
  opt.tool_options("compiler_cc")

def configure(conf):
  conf.check_tool("compiler_cxx")
  conf.check_tool("compiler_cc")
  conf.check_tool("node_addon")

  conf.env.append_value("LIBPATH_MPOOL", abspath("./deps/mpool-2.1.0/"))
  conf.env.append_value("LIB_MPOOL",     "mpool")
  conf.env.append_value("CPPPATH_MPOOL", abspath("./deps/mpool-2.1.0/"))

  conf.env.append_value('LIBPATH_SQLITE', abspath('build/default/deps/sqlite/'))
  conf.env.append_value('STATICLIB_SQLITE', 'sqlite3-bundled')
  conf.env.append_value('CPATH_SQLITE', abspath('./deps/sqlite/'))

def build(bld):
  system("cd deps/mpool-2.1.0/; make");

  sqlite = bld.new_task_gen('cc', 'staticlib')
  sqlite.ccflags = ["-g", "-fPIC", "-D_FILE_OFFSET_BITS=64", "-D_LARGEFILE_SOURCE", "-Wall"]
  sqlite.source = "deps/sqlite/sqlite3.c"
  sqlite.target = "deps/sqlite/sqlite3-bundled"
  sqlite.name = "sqlite3"

  obj = bld.new_task_gen("cxx", "shlib", "node_addon")
  obj.cxxflags = ["-g", "-D_FILE_OFFSET_BITS=64", "-D_LARGEFILE_SOURCE", "-Wall"]
  obj.target = "sqlite3_bindings"
  obj.source = "src/sqlite3_bindings.cc src/database.cc src/statement.cc"
  obj.uselib = "MPOOL"
  obj.uselib_local = "sqlite3"

t = 'sqlite3_bindings.node'
def shutdown():
  # HACK to get binding.node out of build directory.
  # better way to do this?
  if Options.commands['clean']:
    if exists(t): unlink(t)
    system("cd deps/mpool-2.1.0/; make clean");
  else:
    if exists('build/default/' + t) and not exists(t):
      symlink('build/default/' + t, t)

