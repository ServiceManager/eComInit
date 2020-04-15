Checks:          '-*,readability-identifier-naming'
WarningsAsErrors: ''
HeaderFilterRegex: ''
AnalyzeTemporaryDtors: false
CheckOptions:
  - key:             readability-identifier-naming.EnumConstantCase
    value:           'CamelCase'
  - key:             readability-identifier-naming.TypedefCase
    value:           'CamelCase'
  - key:             readability-identifier-naming.GlobalFunctionCase
    value:           'CamelCase'

  - key:             readability-identifier-naming.EnumConstantPrefix
    value:           'k%PREFIX%'

  - key:             readability-identifier-naming.TypedefPrefix
    value:           '%PREFIX%'
  - key:             readability-identifier-naming.GlobalFunctionPrefix
    value:           '%PREFIX%'
