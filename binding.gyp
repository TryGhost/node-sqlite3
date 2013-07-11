{
  'includes': [ 'deps/common-sqlite.gypi' ],
  'variables': {
      'sqlite%':'internal',
  },
  'conditions': [
      ['OS=="win"', {
        'variables': {
          'copy_command%': 'copy',
        },
      },{
        'variables': {
          'copy_command%': 'cp',
        },
      }]
  ],
  'targets': [
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
