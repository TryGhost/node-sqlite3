{
  'conditions': [
      ['OS=="win"', {
        'variables': {
          'copy_command%': 'copy'
        },
      },{
        'variables': {
          'copy_command%': 'cp'
        },
      }]
  ],
  'targets': [
    {
      'target_name': 'node_sqlite3',
      'sources': [
        'src/database.cc',
        'src/node_sqlite3.cc',
        'src/statement.cc'
      ],
      'dependencies': [
        'deps/sqlite3/binding.gyp:sqlite3'
      ]
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
