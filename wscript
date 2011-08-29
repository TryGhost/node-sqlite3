import os
import sys
import Options
from Configure import ConfigurationError
from os.path import exists
from shutil import copy2 as copy, rmtree

# node-wafadmin
import Options
import Utils


TARGET = 'sqlite3_bindings'
TARGET_FILE = '%s.node' % TARGET
built = 'build/default/%s' % TARGET_FILE
dest = 'lib/%s' % TARGET_FILE

BUNDLED_SQLITE3_VERSION = '3070701'
BUNDLED_SQLITE3 = 'sqlite-autoconf-%s' % BUNDLED_SQLITE3_VERSION
BUNDLED_SQLITE3_TAR = 'sqlite-autoconf-%s.tar.gz' % BUNDLED_SQLITE3_VERSION
SQLITE3_TARGET = 'deps/%s' % BUNDLED_SQLITE3

def set_options(opt):
  opt.tool_options("compiler_cxx")

  opt.add_option( '--internal-sqlite'
                , action='store_true'
                , default=True
                , help='Build dynamically against external install of libsqlite3 (default False - build uses internal copy)'
                , dest='internal_sqlite'
                )

def configure(conf):
  conf.check_tool("compiler_cxx")
  conf.check_tool("node_addon")
  

  if not Options.options.internal_sqlite:

      try:
        conf.check_cfg(package="sqlite3", args='--libs --cflags',
                       uselib_store="SQLITE3", mandatory=True)
      except ConfigurationError:
        conf.check(lib="sqlite3", libpath=['/usr/local/lib', '/opt/local/lib'],
                   uselib_store="SQLITE3", mandatory=True)

      Utils.pprint('YELLOW','Note: pass --internal-sqlite to compile and link against bundled sqlite (version %s)' % BUNDLED_SQLITE3_VERSION)
  
  else:
      configure_interal_sqlite3(conf) 

  linkflags = []
  if os.environ.has_key('LINKFLAGS'):
      linkflags.extend(os.environ['LINKFLAGS'].split(' '))
  
  if Options.options.internal_sqlite and Options.platform == 'darwin':
      linkflags.append('-Wl,-search_paths_first')
  
  conf.env.append_value("LINKFLAGS", linkflags)

def configure_interal_sqlite3(conf):
      Utils.pprint('GREEN','Using internal sqlite3!')
      
      os.chdir('deps')
      if not os.path.exists(BUNDLED_SQLITE3):
          os.system('tar xvf %s' % BUNDLED_SQLITE3_TAR)
      os.chdir(BUNDLED_SQLITE3)
      cxxflags = ''
      if os.environ.has_key('CFLAGS'):
          cxxflags += os.environ['CFLAGS']
          cxxflags += ' '
      if os.environ.has_key('CXXFLAGS'):
          cxxflags += os.environ['CXXFLAGS']
      # LINKFLAGS appear to be picked up automatically...
      if not os.path.exists('config.status'):
          os.system("CFLAGS='%s -DSQLITE_ENABLE_RTREE=1 -fPIC -O3 -DNDEBUG' ./configure --disable-dependency-tracking --enable-static --disable-shared" % cxxflags)
      os.chdir('../../')
      
      conf.env.append_value("CPPPATH_SQLITE3", ['../deps/%s' % BUNDLED_SQLITE3])
      conf.env.append_value("LINKFLAGS", ['-L../deps/%s/.libs' % BUNDLED_SQLITE3, '-lsqlite3'])
      
def build_internal_sqlite3():
    if not Options.commands['clean'] and Options.options.internal_sqlite:
        if not os.path.exists(SQLITE3_TARGET):
            Utils.pprint('RED','Please re-run ./configure or node-waf configure')
            sys.exit()
        os.chdir(SQLITE3_TARGET)
        os.system('make')
        os.chdir('../../')

def clean_internal_sqlite3():
    if os.path.exists(SQLITE3_TARGET):
        rmtree(SQLITE3_TARGET)

def build(bld):
  obj = bld.new_task_gen("cxx", "shlib", "node_addon")
  build_internal_sqlite3()
  obj.cxxflags = ["-g", "-D_FILE_OFFSET_BITS=64", "-D_LARGEFILE_SOURCE",
                  "-DSQLITE_ENABLE_RTREE=1", "-pthread", "-Wall"]
  # uncomment the next line to remove '-undefined dynamic_lookup' 
  # in order to review linker errors (v8, libev/eio references can be ignored)
  #obj.env['LINKFLAGS_MACBUNDLE'] = ['-bundle']
  obj.target = TARGET
  obj.source = "src/sqlite3.cc src/database.cc src/statement.cc"
  obj.uselib = "SQLITE3"

def shutdown():
  if Options.commands['clean']:
      if exists(TARGET_FILE):
        unlink(TARGET_FILE)
      clean_internal_sqlite3()
  else:
    if exists(built):
      copy(built, dest)
