{
  'includes': [ 'deps/common-sqlite.gypi' ],
  'variables': {
      'sqlite%':'internal',
  },
  'targets': [
    {
      'target_name': 'node_sqlite3',
      'conditions': [
        ['sqlite != "internal"', {
            'libraries': [
               '-L<@(sqlite)/lib',
               '-lsqlite3'
            ],
            'include_dirs': [ '<@(sqlite)/include' ],
            'conditions': [ [ 'OS=="linux"', {'libraries+':['-Wl,-rpath=<@(sqlite)/lib']} ] ]
        },
        {
            'dependencies': [
              'deps/sqlite3.gyp:sqlite3'
            ]
        }
        ]
      ],
      'sources': [
        'src/database.cc',
        'src/node_sqlite3.cc',
        'src/statement.cc'
      ],
    }
  ]
}
