{
  'includes': [ 'deps/common-sqlite.gypi' ],
  'variables': {
      'sqlite%':'internal',
  },
  'targets': [
    {
      'target_name': 'action_before_build',
      'type': 'none',
      'actions': [
        {
          'action_name': 'unpack_sqlite_dep',
          'inputs': [
            './deps/sqlite-autoconf-<@(sqlite_version).tar.gz'
          ],
          'outputs': [
            './deps/sqlite-autoconf-<@(sqlite_version)'
          ],
          'action': ['./node_modules/.bin/targz','deps/sqlite-autoconf-<@(sqlite_version).tar.gz','-x','deps/']
        }
      ]
    },
    {
      'target_name': 'node_sqlite3',
      'conditions': [
        ['sqlite != "internal"', {
            'libraries': [
               '-L<@(sqlite)/lib',
               '-lsqlite3'
            ],
            'include_dirs': [ '<@(sqlite)/include' ]
        },
        {
            'dependencies': [
              'action_before_build',
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
