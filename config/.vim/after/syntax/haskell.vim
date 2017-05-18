" Extension to Haskell syntax
" Language: C
" Maintainer: Matthew Fernandez <matthew.fernandez@gmail.com>
" License: Public domain

" Yes, I know these aren't statements, but I enjoy thinking of them as such.
syn match hsStatement "\<\(return\|unless\|when\)\>"

" Some constants
syn keyword hsConstant_ otherwise stdin stderr stdout
highlight link hsConstant_ Number

syn keyword hsOperator not
