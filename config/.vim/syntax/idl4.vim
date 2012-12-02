syn keyword IDLType trait interface in out inout
syn keyword IDLCType int string char void unsigned signed integer character float double real boolean bool pointer int8_t int16_t int32_t int64_t uint8_t uint16_t uint32_t uint64_t
syn region Foldable start="{" end="}" fold transparent
syn match IDLMultiLineComment "\/\*\_.\{-}\*\/"
syn match IDLSingleLineComment "\/\/.*$"
syn region IDLString start='"' end='"'

hi def link IDLType Type
hi def link IDLCType Type
hi def link IDLMultiLineComment Comment
hi def link IDLSingleLineComment Comment
hi def link IDLString Constant
