{
  "includes": [ "deps/common-sqlite.gypi" ],
  "variables": {
      "sqlite%":"internal",
      "module_name":"node_sqlite3",
      "module_path":"./lib/"
  },
  "targets": [
    {
      "target_name": "<(module_name)",
      "conditions": [
        ["sqlite != 'internal'", {
            "libraries": [
               "-L<@(sqlite)/lib",
               "-lsqlite3"
            ],
            "include_dirs": [ "<@(sqlite)/include" ],
            "conditions": [ [ "OS=='linux'", {"libraries+":["-Wl,-rpath=<@(sqlite)/lib"]} ] ]
        },
        {
            "dependencies": [
              "deps/sqlite3.gyp:sqlite3"
            ]
        }
        ]
      ],
      "sources": [
        "src/database.cc",
        "src/node_sqlite3.cc",
        "src/statement.cc"
      ]
    },
    {
      "target_name": "action_after_build",
      "type": "none",
      "dependencies": [ "<(module_name)" ],
      "copies": [
          {
            "files": [ "<(PRODUCT_DIR)/<(module_name).node" ],
            "destination": "<(module_path)"
          }
      ]
    }
  ]
}
