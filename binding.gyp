{
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
    }
  ]
}
