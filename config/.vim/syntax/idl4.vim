syn keyword IDLType interface in out
syn keyword IDLCType int string smallstring char void unsigned signed long
syn region Foldable start="{" end="}" fold transparent
syn match IDLMultiLineComment "\/\*\_.\{-}\*\/"
syn match IDLSingleLineComment "\/\/.*$"
syn region IDLString start='"' end='"'

hi def link IDLType Type
hi def link IDLCType Type
hi def link IDLMultiLineComment Comment
hi def link IDLSingleLineComment Comment
hi def link IDLString Constant
