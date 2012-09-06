syn keyword CamkesKeyword assembly composition from to configuration control
syn keyword CamkesType component connection attribute
syn keyword CamkesCType int string smallstring char unsigned signed void long
syn keyword CamkesDependency uses provides emits consumes
syn keyword CamkesImport import
syn region Foldable start="{" end="}" fold transparent
syn match CamkesMultiLineComment "\/\*\_.\{-}\*\/"
syn match CamkesSingleLineComment "\/\/.*$"
syn region CamkesString start='"' end='"'

hi def link CamkesKeyword Statement
hi def link CamkesType Type
hi def link CamkesCType Type
hi def link CamkesDependency Type
hi def link CamkesImport PreProc
hi def link CamkesMultiLineComment Comment
hi def link CamkesSingleLineComment Comment
hi def link CamkesString Constant
