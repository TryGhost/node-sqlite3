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
      "xcode_settings": { "GCC_ENABLE_CPP_EXCEPTIONS": "YES",
        "CLANG_CXX_LIBRARY": "libc++",
        "MACOSX_DEPLOYMENT_TARGET": "10.7",
      },
      "msvs_settings": {
        "VCCLCompilerTool": { "ExceptionHandling": 1 },
      },
      "conditions": [
        ["OS == 'emscripten'", {
          # make require('xxx.node') work
          'product_extension': 'node.js',
          'defines': [
            # UV_THREADPOOL_SIZE equivalent
            'EMNAPI_WORKER_POOL_SIZE=4',
          ],
          "ldflags": [
            '-sMODULARIZE=1',
            '-sEXPORT_NAME=nodeSqlite3',
            # building only for Node.js for now
            "-sNODERAWFS=1",
            '-sWASM_ASYNC_COMPILATION=0',
          ],
          "dependencies": [
            "deps/sqlite3.gyp:sqlite3"
          ]
        }, {
          "conditions": [
            ["sqlite != 'internal'", {
              "include_dirs": [
                "<!@(node -p \"require('node-addon-api').include\")", "<(sqlite)/include" ],
              "libraries": [
                "-l<(sqlite_libname)"
              ],
              "conditions": [
                [ "OS=='linux'", {"libraries+":["-Wl,-rpath=<@(sqlite)/lib"]} ],
                [ "OS!='win'", {"libraries+":["-L<@(sqlite)/lib"]} ]
              ],
              'msvs_settings': {
                'VCLinkerTool': {
                  'AdditionalLibraryDirectories': [
                    '<(sqlite)/lib'
                  ],
                },
              }
            }, {
              "include_dirs": [
                "<!@(node -p \"require('node-addon-api').include\")"
              ],
              "dependencies": [
                "<!(node -p \"require('node-addon-api').gyp\")",
                "deps/sqlite3.gyp:sqlite3"
              ]
            }]
          ]
        }],
      ],
      "sources": [
        "src/backup.cc",
        "src/database.cc",
        "src/node_sqlite3.cc",
        "src/statement.cc"
      ],
      "defines": [ "NAPI_VERSION=<(napi_build_version)", "NAPI_DISABLE_CPP_EXCEPTIONS=1" ]
    },
    {
      "target_name": "action_after_build",
      "type": "none",
      "dependencies": [ "<(module_name)" ],
      "conditions": [
        ["OS == 'emscripten'", {
          "copies": [
            {
              "files": [
                "<(PRODUCT_DIR)/<(module_name).node.js",
                "<(PRODUCT_DIR)/<(module_name).node.wasm",
                "<(PRODUCT_DIR)/<(module_name).node.worker.js",
              ],
              "destination": "<(module_path)"
            }
          ]
        }, {
          "copies": [
            {
              "files": [ "<(PRODUCT_DIR)/<(module_name).node" ],
              "destination": "<(module_path)"
            }
          ]
        }]
      ]
    }
  ]
}
