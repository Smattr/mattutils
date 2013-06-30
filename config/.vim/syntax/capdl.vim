syn keyword CapDLKeyword arch caps objects arm11 ia32
syn keyword CapDLObject aep asid_pool cnode ep frame pd pt tcb
syn keyword CapDLAttribute addr badge elf G guard guard_size init ip prio sp R RG RW RWG W WG
syn match CapDLCPP "[ \t]*#.*$"
syn match CapDLLiteral "\(0x\)\?[0-9]\+k\?\( bits\)\?"
syn match CapDLLiteral "0x[0-f]\+"

syn region Foldable start="{" end="}" fold transparent

syn match CapDLMultiLineComment "\/\*\_.\{-}\*\/"
syn match CapDLSingleLineComment "\/\/.*$"

hi def link CapDLMultiLineComment Comment
hi def link CapDLSingleLineComment Comment
hi def link CapDLKeyword Statement
hi def link CapDLObject Type
hi def link CapDLAttribute Type
hi def link CapDLCPP PreProc
hi def link CapDLLiteral Constant
