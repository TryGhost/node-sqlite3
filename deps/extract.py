import sys
import tarfile
import os

tarball = os.path.abspath(sys.argv[1])
dirname = os.path.abspath(sys.argv[2])
tfile = tarfile.open(tarball,'r:gz');
tfile.extractall(dirname)

# amalgamate extension-functions.c under flag SQLITE_ENABLE_JSON1
with open(os.path.join(sys.argv[3], 'extension-functions.c')) as file:
  src_extension_functions = file.read()
  src_extension_functions = src_extension_functions.replace(
    'typedef uint8_t',
    '//typedef uint8_t',
    1
  )
  src_extension_functions = src_extension_functions.replace(
    'typedef uint16_t',
    '//typedef uint16_t',
    1
  )
  src_extension_functions = src_extension_functions.replace(
    'typedef int64_t',
    '//typedef int64_t',
    1
  )
  src_extension_functions = src_extension_functions.replace(
    'READ_UTF8',
    'READ_UTF8_2',
  )
  src_extension_functions = src_extension_functions.replace(
    'sqlite3Utf8CharLen',
    'sqlite3Utf8CharLen2'
  )
with open(os.path.join(sys.argv[4], 'sqlite3.c')) as file:
  src_sqlite3 = file.read()
  src_sqlite3 = src_sqlite3.replace(
    '/************** End of json1.c ***********************************************/\n',
    '/************** End of json1.c ***********************************************/\n' + src_extension_functions + '\n',
    1
  )
  src_sqlite3 = src_sqlite3.replace(
    'SQLITE_PRIVATE int sqlite3Json1Init(sqlite3*);\n',
    (
        'SQLITE_PRIVATE int sqlite3Json1Init(sqlite3*);\n' +
        'SQLITE_PRIVATE int RegisterExtensionFunctions(sqlite3*);\n'
    ),
    1
  )
  src_sqlite3 = src_sqlite3.replace(
    'sqlite3Json1Init,',
    'sqlite3Json1Init,RegisterExtensionFunctions,',
    1
  )
with open(os.path.join(sys.argv[4], 'sqlite3.c'), 'w') as file:
  file.write(src_sqlite3)

sys.exit(0)
