{
  'targets': [
    {
      'target_name': 'node_sqlite3',
      'sources': [
        'src/database.cc',
        'src/node_sqlite3.cc',
        'src/statement.cc'
      ],
      'libraries': [ '-L/opt/local/lib', '-lsqlite3' ]
    }
  ]
}
