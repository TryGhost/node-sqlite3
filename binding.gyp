{
  "includes": [ "deps/common-sqlite.gypi" ],
  "variables": {
      "sqlite%":"internal",
      "sqlite_libname%":"sqlite3"
  },
  "targets": [
    {
      "target_name": "<(module_name)",
      "cflags!": [ "-fno-exceptions" ],
      "cflags_cc!": [ "-fno-exceptions" ],
      "include_dirs": ["<!@(node -p \"require('node-addon-api').include\")"],
      "dependencies": ["<!(node -p \"require('node-addon-api').gyp\")"],
      "conditions": [
        ["sqlite != 'internal'", {
            "include_dirs": [ "<(sqlite)/include" ],
            "libraries": [
               "-l<(sqlite_libname)"
            ],
            "conditions": [ [ "OS=='linux'", {"libraries+":["-Wl,-rpath=<@(sqlite)/lib"]} ] ],
            "conditions": [ [ "OS!='win'", {"libraries+":["-L<@(sqlite)/lib"]} ] ],
            'msvs_settings': {
              'VCLinkerTool': {
                'AdditionalLibraryDirectories': [
                  '<(sqlite)/lib'
                ],
              },
            }
        },
        {
            "dependencies": [
              "deps/sqlite3.gyp:sqlite3"
            ]
        }
        ]
      ],
      'xcode_settings': {
        'MACOSX_DEPLOYMENT_TARGET':'10.8',
        'CLANG_CXX_LIBRARY': 'libc++',
        'CLANG_CXX_LANGUAGE_STANDARD':'c++11',
        'GCC_VERSION': 'com.apple.compilers.llvm.clang.1_0'
      },
      'defines': [ 'NAPI_DISABLE_CPP_EXCEPTIONS' ],
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
