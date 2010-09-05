import Options
from os import unlink, symlink, system
from os.path import exists, abspath

srcdir = "."
blddir = "build"
VERSION = "0.0.1"

def set_options(opt):
  opt.tool_options("compiler_cxx")

def configure(conf):
  conf.check_tool("compiler_cxx")
  conf.check_tool("node_addon")
  if not conf.check_cfg(package='sqlite3', args='--cflags --libs', uselib_store='SQLITE3'):
    if not conf.check(lib="sqlite3", libpath=['/usr/local/lib', '/opt/local/lib'], uselib_store="SQLITE3"):
      conf.fatal('Missing sqlite3');
  conf.env.append_value('LIBPATH_SQLITE3', '/opt/local/lib');
#   conf.check_cfg(package='profiler', args='--cflags --libs', uselib_store='SQLITE3')
#   conf.env.append_value('LIBPATH_PROFILER', '/usr/local/lib')
#   conf.env.append_value('LIB_PROFILER', 'profiler')

  conf.env.append_value("LIBPATH_MPOOL", abspath("./deps/mpool-2.1.0/"))
  conf.env.append_value("LIB_MPOOL",     "mpool")
  conf.env.append_value("CPPPATH_MPOOL", abspath("./deps/mpool-2.1.0/"))


def build(bld):
  system("cd deps/mpool-2.1.0/; make");
  obj = bld.new_task_gen("cxx", "shlib", "node_addon")
  obj.cxxflags = ["-g", "-D_FILE_OFFSET_BITS=64", "-D_LARGEFILE_SOURCE", "-Wall"]
  obj.target = "sqlite3_bindings"
  obj.source = "src/sqlite3_bindings.cc src/database.cc src/statement.cc"
  obj.uselib = "SQLITE3 PROFILER MPOOL"

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

