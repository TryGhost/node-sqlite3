{
  "includes": [ "deps/common-sqlite.gypi" ],
  "variables": {
      "sqlite%":"internal",
      "sqlite_libname%":"sqlite3"
  },
  "targets": [
    {
      "target_name": "<(module_name)",
      "include_dirs": ["<!(node -e \"require('nan')\")"],
      "conditions": [
        ["sqlite != 'internal'", {
            "libraries": [
               "-L<@(sqlite)/lib",
               "-l<(sqlite_libname)"
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
