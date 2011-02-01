# vim: ft=javascript
import Options
from os import unlink, system
from os.path import exists, abspath
from shutil import copy2 as copy

TARGET = 'sqlite3_bindings'
TARGET_FILE = '%s.node' % TARGET
built = 'build/default/%s' % TARGET_FILE
dest = 'lib/%s' % TARGET_FILE

def set_options(opt):
  opt.tool_options("compiler_cxx")
  opt.tool_options("compiler_cc")

def configure(conf):
  conf.check_tool("compiler_cxx")
  conf.check_tool("compiler_cc")
  conf.check_tool("node_addon")
  if not conf.check_cfg(package='sqlite3', args='--cflags --libs', uselib_store='SQLITE3'):
    if not conf.check(lib="sqlite3", libpath=['/usr/local/lib', '/opt/local/lib'], uselib_store="SQLITE3"):
      conf.fatal('Missing sqlite3');
  conf.env.append_value('LIBPATH_SQLITE3', '/opt/local/lib');

  conf.env.append_value("LIBPATH_MPOOL", abspath("./deps/mpool-2.1.0/"))
  conf.env.append_value("LIB_MPOOL",     "mpool")
  conf.env.append_value("CPPPATH_MPOOL", abspath("./deps/mpool-2.1.0/"))

  conf.env.append_value('LIBPATH_SQLITE', abspath('build/default/deps/sqlite/'))
  conf.env.append_value('STATICLIB_SQLITE', 'sqlite3-bundled')
  conf.env.append_value('CPATH_SQLITE', abspath('./deps/sqlite/'))

def build(bld):
  system("cd deps/mpool-2.1.0/; make");
  obj = bld.new_task_gen("cxx", "shlib", "node_addon", install_path=None)
  obj.cxxflags = ["-g", "-D_FILE_OFFSET_BITS=64", "-D_LARGEFILE_SOURCE", "-Wall"]
  obj.target = TARGET
  obj.source = "src/sqlite3_bindings.cc"
  obj.source += " src/database.cc"
  obj.source += " src/statement.cc"
  obj.uselib = "SQLITE3 PROFILER MPOOL"
  start_dir = bld.path.find_dir('lib')
  # http://www.freehackers.org/~tnagy/wafbook/index.html#_installing_files
  bld.install_files('${PREFIX}/lib/node/sqlite', start_dir.ant_glob('*'), cwd=start_dir, relative_trick=True)

def shutdown():
  if Options.commands['clean']:
    if exists(TARGET_FILE):
      unlink(TARGET_FILE)
    system("cd deps/mpool-2.1.0/; make clean");
  else:
    if exists(built):
      copy(built, dest)

