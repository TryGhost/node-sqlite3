{
  'includes': [ 'deps/common-sqlite.gypi' ],
  'variables': {
      'sqlite%':'internal',
  },
  'conditions': [
      ['OS=="win"', {
        'variables': {
          'copy_command%': 'copy',
          'bin_name':'call'
        },
      },{
        'variables': {
          'copy_command%': 'cp',
          'bin_name':'node'
        },
      }]
  ],
  'targets': [
    {
      'target_name': 'action_before_build',
      'type': 'none',
      'conditions': [
        ['sqlite == "internal"', {
          'actions': [
            {
              'action_name': 'unpack_sqlite_dep',
              'inputs': [
                './deps/sqlite-autoconf-<@(sqlite_version).tar.gz'
              ],
              'outputs': [
                './deps/sqlite-autoconf-<@(sqlite_version)/sqlite3.c'
              ],
              'action': ['<@(bin_name)','./node_modules/.bin/targz','deps/sqlite-autoconf-<@(sqlite_version).tar.gz','-x','deps/']
            }
          ]
        }]
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
    },
    {
      'target_name': 'action_after_build',
      'type': 'none',
      'dependencies': [ 'node_sqlite3' ],
      'actions': [
        {
          'action_name': 'move_node_module',
          'inputs': [
            '<@(PRODUCT_DIR)/node_sqlite3.node'
          ],
          'outputs': [
            'lib/node_sqlite3.node'
          ],
          'action': ['<@(copy_command)', '<@(PRODUCT_DIR)/node_sqlite3.node', 'lib/node_sqlite3.node']
        }
      ]
    }
  ]
}
