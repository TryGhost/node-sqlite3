# This file is originally created by [RReverser](https://github.com/RReverser)
# in https://github.com/lovell/sharp/pull/3522
{
  'variables': {
    'OS': 'emscripten',
    'napi_build_version%': '8',
    'clang': 1,
    'target_arch%': 'wasm32',
    'wasm_threads%': 1,
    'product_extension%': 'js',
  },

  'target_defaults': {
    'type': 'executable',
    'product_extension': '<(product_extension)',

    'cflags': [
      '-Wall',
      '-Wextra',
      '-Wno-unused-parameter',
      '-sDEFAULT_TO_CXX=0',
    ],
    'cflags_cc': [
      '-fno-rtti',
      '-fno-exceptions',
      '-std=gnu++17'
    ],
    'ldflags': [
      '--js-library=<!(node -p "require(\'@tybys/emnapi\').js_library")',
      "-sALLOW_MEMORY_GROWTH=1",
      "-sEXPORTED_FUNCTIONS=['_malloc','_free']",
      '-sNODEJS_CATCH_EXIT=0',
      '-sNODEJS_CATCH_REJECTION=0',
      '-sAUTO_JS_LIBRARIES=0',
      '-sAUTO_NATIVE_LIBRARIES=0',
    ],
    'defines': [
      '__STDC_FORMAT_MACROS',
    ],
    'include_dirs': [
      '<!(node -p "require(\'@tybys/emnapi\').include")',
    ],
    'sources': [
      '<!@(node -p "require(\'@tybys/emnapi\').sources.map(x => JSON.stringify(path.relative(process.cwd(), x))).join(\' \')")'
    ],

    'default_configuration': 'Release',
    'configurations': {
      'Debug': {
        'defines': [ 'DEBUG', '_DEBUG' ],
        'cflags': [ '-g', '-O0' ],
        'ldflags': [ '-g', '-O0', '-sSAFE_HEAP=1' ],
      },
      'Release': {
        'cflags': [ '-O3' ],
        'ldflags': [ '-O3' ],
      }
    },

    'conditions': [
      ['target_arch == "wasm64"', {
        'cflags': [
          '-sMEMORY64=1',
        ],
        'ldflags': [
          '-sMEMORY64=1'
        ]
      }],
      ['wasm_threads == 1', {
        'cflags': [ '-sUSE_PTHREADS=1' ],
        'ldflags': [ '-sUSE_PTHREADS=1' ],
      }],
    ],
  }
}
