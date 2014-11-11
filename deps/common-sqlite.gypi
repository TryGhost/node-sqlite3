{
  'variables': {
      'sqlite_version%':'3080701',
      "toolset%":'',
  },
  'target_defaults': {
    'default_configuration': 'Release',
    'msbuild_toolset':'<(toolset)'
  }
}