{
  'includes': [ 'common-sqlite.gypi' ],
  'target_defaults': {
    'default_configuration': 'Debug',
    'configurations': {
      'Debug': {
        'defines': [ 'DEBUG', '_DEBUG' ],
        'msvs_settings': {
          'VCCLCompilerTool': {
            'RuntimeLibrary': 1, # static debug
          },
        },
      },
      'Release': {
        'defines': [ 'NDEBUG' ],
        'msvs_settings': {
          'VCCLCompilerTool': {
            'RuntimeLibrary': 0, # static release
          },
        },
      }
    },
    'msvs_settings': {
      'VCCLCompilerTool': {
      },
      'VCLibrarianTool': {
      },
      'VCLinkerTool': {
        'GenerateDebugInformation': 'true',
      },
    },
    'conditions': [
      ['OS == "win"', {
        'defines': [
          'WIN32'
        ],
      }]
    ],
  },

  'targets': [
    {
      'target_name': 'sqlite3',
      'type': 'static_library',
      'include_dirs': [ './sqlite-autoconf-<@(sqlite_version)/' ],
      'direct_dependent_settings': {
        'include_dirs': [ './sqlite-autoconf-<@(sqlite_version)/' ],
        'defines': [
          'SQLITE_THREADSAFE=1',
          'SQLITE_ENABLE_FTS3',
          'SQLITE_ENABLE_RTREE'
        ],
      },
      'defines': [
        '_REENTRANT=1',
        'SQLITE_THREADSAFE=1',
        'SQLITE_ENABLE_FTS3',
        'SQLITE_ENABLE_RTREE'
      ],
      'sources': [ './sqlite-autoconf-<@(sqlite_version)/sqlite3.c' ],
    }
  ]
}
