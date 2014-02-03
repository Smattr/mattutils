" Vim syntax for ADL. Copy this to ~/.vim/syntax/ and add the following line to
" ~/.vim/filetype.vim:
"
"  augroup filetypedetect
"      au BufRead,BufNewFile *.camkes setfiletype camkes
"  augroup END

syn match CamkesCPP "[ \t]*#.*$"
syn keyword CamkesKeyword assembly composition from to configuration control
    \ procedure hardware maybe flatten dma_pool has mutex semaphore
syn keyword CamkesType component connection attribute connector
syn keyword CamkesCType int string smallstring char unsigned signed void long
syn keyword CamkesDependency uses provides emits consumes
syn keyword CamkesImport import include
syn region Foldable start="{" end="}" fold transparent
syn match CamkesMultiLineComment "\/\*\_.\{-}\*\/"
syn match CamkesSingleLineComment "\/\/.*$"
syn region CamkesString start='"' end='"'
syn region CamkesBuiltin start='<' end='>'

hi def link CamkesCPP PreProc
hi def link CamkesKeyword Statement
hi def link CamkesType Type
hi def link CamkesCType Type
hi def link CamkesDependency Type
hi def link CamkesImport PreProc
hi def link CamkesMultiLineComment Comment
hi def link CamkesSingleLineComment Comment
hi def link CamkesString Constant
hi def link CamkesBuiltin Constant
