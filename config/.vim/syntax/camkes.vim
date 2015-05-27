"
" Copyright 2014, NICTA
"
" This software may be distributed and modified according to the terms of
" the BSD 2-Clause license. Note that NO WARRANTY is provided.
" See "LICENSE_BSD2.txt" for details.
"
" @TAG(NICTA_BSD)
"

" Vim syntax for ADL. Copy this to ~/.vim/syntax/ and add the following line to
" ~/.vim/filetype.vim:
"
"  augroup filetypedetect
"      au BufRead,BufNewFile *.camkes setfiletype camkes
"  augroup END

syn match CamkesCPP "^[ \t]*#.*$"
syn keyword CamkesKeyword assembly composition from to configuration control
    \ procedure hardware maybe dma_pool has mutex semaphore group tcb_pool
    \ ep_pool aep_pool from_access to_access template untyped_mmio trusted
    \ cnode_size_bits
syn match CamkesUntypedPool "untyped[0-9]\+_pool"
syn match CamkesStackSize "[a-zA-Z0-9_]\+_stack_size"
syn match CamkesPriority "\<\(priority\|[a-zA-Z][a-zA-Z0-9_]*_priority\|_control_priority\)\>"
syn keyword CamkesType component connection attribute connector Procedure Event
    \ Dataport
syn keyword CamkesCType int string char character unsigned signed
    \ void long refin in out inout int8_t uint8_t int16_t uint16_t int32_t uint32_t
    \ int64_t uint64_t
syn keyword CamkesDependency uses provides emits consumes
syn keyword CamkesImport import include
syn region Foldable start="{" end="}" fold transparent
syn match CamkesMultiLineComment "\/\*\_.\{-}\*\/"
syn match CamkesSingleLineComment "\/\/.*$"
syn region CamkesString start='"' end='"'
syn region CamkesBuiltin start='<' end='>'
syn match CamkesNumber "\<\(0x\x\+\|-\?\d\+\(\.\d\+\)\?\)\>"

hi def link CamkesCPP PreProc
hi def link CamkesKeyword Statement
hi def link CamkesUntypedPool Statement
hi def link CamkesStackSize Statement
hi def link CamkesPriority Statement
hi def link CamkesType Type
hi def link CamkesCType Type
hi def link CamkesDependency Type
hi def link CamkesImport PreProc
hi def link CamkesMultiLineComment Comment
hi def link CamkesSingleLineComment Comment
hi def link CamkesString Constant
hi def link CamkesBuiltin Constant
hi def link CamkesNumber Constant
